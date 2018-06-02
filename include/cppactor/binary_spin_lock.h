/***************************************************************************
 *    
 *                  Unpublished Work Copyright (c) 2007-2011
 *                  Trading Technologies International, Inc.
 *                       All Rights Reserved Worldwide
 *
 *          * * *   S T R I C T L Y   P R O P R I E T A R Y   * * *
 *
 * WARNING:  This program (or document) is unpublished, proprietary property
 * of Trading Technologies International, Inc. and is to be maintained in
 * strict confidence. Unauthorized reproduction, distribution or disclosure 
 * of this program (or document), or any program (or document) derived from
 * it is prohibited by State and Federal law, and by local law outside of 
 * the U.S.
 *
 ***************************************************************************/


#pragma once

#ifndef TT_BINARY_SPIN_LOCK_H_INCLUDED
#define TT_BINARY_SPIN_LOCK_H_INCLUDED

/***********************************************************************************************************/

#include <atomic>
#include <chrono>
#include "noncopyable.h"

namespace miscutils
{
    //! @class SimpleSpinLock
    //! This implementation of a SimpleSpinLock uses (long) values for a simple
    //! binary inc/dec.
    //! This implementation is should be used in cases where non-blocking threads
    //! will need to unlock and the wait time will not be prolonged.
    //! @anchor SimpleSpinLock
    class SimpleSpinLock 
    {
    	NONCOPYABLE_CLASS(SimpleSpinLock);
    public:

        //! Creates a new lock.
        SimpleSpinLock() throw();

        //! Acquires the lock for use, will wait if the lock is already in use.
        //! Increments long value by one in an atomic way to indicate in-use.
        void lock() throw();

        //! Releases the lock, decrements counter.
        void unlock() throw();

        //! Returns true if the lock is currently held.
        bool IsLocked() const throw();

    protected:

        enum {SPIN_LOCK_UNLOCKED_VALUE = 0, SPIN_LOCK_IS_LOCKED_VALUE = 1};

        //! Specified if the object is locked or not.
        std::atomic<long> m_locked;
    };

    //! @class DoNothingSpinLock
    //! This class follows the SimpleSpinLock interface but does no work.  It is intended for
    //! cases that require a thin class to fulfill a template argument but when no locking
    //! is necessary.
    //! @anchor DoNothingSpinLock
    class DoNothingSpinLock 
    {
    public:

        //! Creates a new lock.
        DoNothingSpinLock() throw();

        //! Acquire interface.
        void lock() throw();

        //! Releases interface.
        void unlock() throw();
    };

    //! @class BinarySpinLock
    //!
    //! This implementation of a BinarySpinLock allows a user to specify
    //! a time out period after which the spin lock acquisition will give up.
    //! @anchor BinarySpinLock
    class BinarySpinLock : private SimpleSpinLock
    {
    public:

        //! Creates a new BinarySpinLock
        BinarySpinLock( int timeOutSeconds = 2 ) throw();

        //! Acquires the lock for use, will wait if the lock is already in use.
        //! Increments long value by one in an atomic way to indicate in-use.
        void lock() throw();

        using SimpleSpinLock::unlock;
        using SimpleSpinLock::IsLocked;

    private:

        bool DelayedAcquire() throw();

        //! The amount of time in milliseconds to time out an lock
        std::chrono::milliseconds m_timeOutMilliseconds;
    };


    template <typename T>
    class SpinLockMonitor
    {
    public:
    	explicit SpinLockMonitor(T& lock);
    	~SpinLockMonitor();

    private:
    	T& m_lock;
    };

    /////////////////////////////////////////////////////////////////////////////////////
    // Implementation of inline members of SimpleSpinLock
    /////////////////////////////////////////////////////////////////////////////////////

    inline SimpleSpinLock::SimpleSpinLock() throw()
        : m_locked(SPIN_LOCK_UNLOCKED_VALUE)
    {
    }

    inline void SimpleSpinLock::lock() throw()
    {
    	long expected;
    	do
    	{
    		expected = SPIN_LOCK_UNLOCKED_VALUE;
    	} while (!m_locked.compare_exchange_weak(expected, (long)SPIN_LOCK_IS_LOCKED_VALUE));
    }

    inline void SimpleSpinLock::unlock() throw()
    {
    	long expected;
    	do
    	{
    		expected = SPIN_LOCK_IS_LOCKED_VALUE;
    	} while (!m_locked.compare_exchange_weak(expected, (long)SPIN_LOCK_UNLOCKED_VALUE));
    }

    inline bool SimpleSpinLock::IsLocked() const throw()
    {
        return (m_locked > SPIN_LOCK_UNLOCKED_VALUE);
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // Implementation of inline members of DoNothingSpinLock
    /////////////////////////////////////////////////////////////////////////////////////

    inline DoNothingSpinLock::DoNothingSpinLock() throw()
    {
    }

    inline void DoNothingSpinLock::lock() throw()
    {
    }

    inline void DoNothingSpinLock::unlock() throw()
    {
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // Implementation of inline members of BinarySpinLock
    /////////////////////////////////////////////////////////////////////////////////////

    inline BinarySpinLock::BinarySpinLock( int timeOutSeconds ) throw()
        : SimpleSpinLock()
        , m_timeOutMilliseconds(timeOutSeconds * 1000)
    {
    }

    inline void BinarySpinLock::lock() throw()
    {
    	long expected = SPIN_LOCK_UNLOCKED_VALUE;
        if (!m_locked.compare_exchange_weak(expected, SPIN_LOCK_IS_LOCKED_VALUE))
        {
            // we didn't get the lock
            DelayedAcquire();
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // Implementation of inline members of SpinLockMonitor
    /////////////////////////////////////////////////////////////////////////////////////
    template <typename T>
    inline SpinLockMonitor<T>::SpinLockMonitor(T& lock)
    	: m_lock(lock)
    {
    	m_lock.lock();
    }

    template <typename T>
    inline SpinLockMonitor<T>::~SpinLockMonitor()
    {
    	m_lock.unlock();
    }

}

#endif // TT_BINARY_SPIN_LOCK_H_INCLUDED

