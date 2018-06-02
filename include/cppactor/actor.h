/***************************************************************************
 *
 *                    Unpublished Work Copyright (c) 2014
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
#include <atomic>
#include <memory>
#include <cassert>
#include <functional>
#include "cppactor/detail/pool_base.h"
#include <queue>
#include "miscutils/binary_spin_lock.h"

namespace cppactor
{
    class message;
    class framework;

#define private_impl public

    /****************************************************************
     * All actors are derived from actor
     */
    class actor : public instrusive_base
    {
    public:
        actor()
        : type_id(0)
        , actor_id(0)
        , stopped(false)
        {}

        ~actor();

        /* 
         * Send a message to this actor
         */
        unsigned int enqueue(message *);
        
        /*
         * Enqueue a function to be executed
         */
        unsigned int enqueue(std::function<void (cppactor::actor_iptr)>&&);

        /* called when an actor is started for the first time.
         * This is where you would perform any initialization that requires
         * setting timers or sending messages
         */
        virtual void on_start() {}

        /* Call when in actor is terminated and will be destroyed.
         */
        virtual void on_exit() {}

        /* Override if you set any timers for this actor
         */
        virtual void on_timer(int timerid) {}

        uint32_t get_actorid() const {return actor_id;}
        bool is_stopped() {return stopped;}
    protected:
        /* Returns an actor_iptr (instrusive_ptr<actor>) for this. 
         * Derived classes can call this to call api's that require
         * an actor_iptr
         */
        actor_iptr convert_this();
    private_impl: 
        bool consume_one_item(cppactor::message*& pMsg);
        bool requeue();
    private_impl:
        size_t type_id;
        std::queue<message*> m_queue;
        detail::pool_t m_pPool;
        uint32_t actor_id;

        miscutils::SimpleSpinLock m_spin_lock;

        static std::atomic<uint32_t> m_actorids;
    private:
        friend framework;
        void stop() {stopped = true;}
        std::atomic<bool> stopped;
    };

    inline actor_iptr actor::convert_this() {return actor_iptr(this);}

    // comparator for using actor_iptr as a map key
    struct actor_less
    {
        bool operator()(const cppactor::actor_iptr& a1, const cppactor::actor_iptr& a2)
        {
            return a1.get() < a2.get();
        }
    };

    typedef instrusive_ptr<actor> actor_iptr;

    inline unsigned int actor::enqueue(message *pMsg)
    {
        assert(type_id != 0);   // This actor should have been created with cppactor::create_actor<>()
        if (stopped)
            return 0;

        miscutils::SpinLockMonitor<miscutils::SimpleSpinLock> lock(m_spin_lock);
        m_queue.push(pMsg);
        if (m_queue.size()==1)
            m_pPool->notify_one(this);
        else
            m_pPool->notify_one();

        return m_queue.size();// use outside lock for statistical and logging use only
    }

    inline bool actor::consume_one_item(cppactor::message*& pMsg)
    {
        miscutils::SpinLockMonitor<miscutils::SimpleSpinLock> lock(m_spin_lock);
        if (m_queue.empty())
            return false;

        pMsg=m_queue.front();// note no pop
        return true;
    }

    inline bool actor::requeue()
    {
        miscutils::SpinLockMonitor<miscutils::SimpleSpinLock> lock(m_spin_lock);

        // release the last processed msg and see where we are queue wise
        if (m_queue.size())
            m_queue.pop();

        if (!stopped && m_queue.size())
        {
            m_pPool->notify_one(this);
            return true;
        }
        return false;
    }
}   // cppactor


