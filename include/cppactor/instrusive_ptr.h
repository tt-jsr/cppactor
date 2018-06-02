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
#include "instrusive_base.h"

namespace cppactor
{
    template<typename T>
    class instrusive_ptr
    {
    public:
        typedef T         element_type;
        typedef T *       pointer;
        typedef const T * const_pointer;
        typedef T&        reference;
        typedef const T&  const_reference;

        instrusive_ptr()
        :object(nullptr)
        {}

        instrusive_ptr(pointer p)
        :object(nullptr)
        {
            reset(p);
        }

        ~instrusive_ptr()
        {
            dec_ref(object);
        }

        // Copy constructor
        instrusive_ptr(const instrusive_ptr<T>& cc)
        :object(nullptr)
        {
            reset(cc.object);
        }

        // Copy from a derived type
        template<typename U>
        instrusive_ptr(const instrusive_ptr<U>& cc)
        :object(nullptr)
        {
            reset(cc.object);
        }

        instrusive_ptr& operator=(const instrusive_ptr& rhs)
        {
            reset(rhs.object);
            return *this;
        }

        // assignment from a derived type
        template<typename U>
        instrusive_ptr& operator=(const instrusive_ptr<U>& rhs)
        {
            reset(rhs.object);
            return *this;
        }

        // true if not null
        operator bool() {return object != nullptr;}

        // Cast to type U, usually used to downcast to derived type
        // example:
        //       instrusive_ptr<base_class> base;
        //       instrusive_ptr<derived_class> derived = base.cast<derived_class>()
        template<typename U>
        instrusive_ptr<U> cast()
        {
            return instrusive_ptr<U>(static_cast<U *>(object));
        }

        template <typename...Args>
        void emplace(Args&&... args)
        {
            reset(new T(std::forward<Args>(args)...));
        }

        pointer operator->()
        {
            return object;
        }

        const_pointer operator->() const
        {
            return object;
        }

        reference operator *()
        {
            return *object;
        }

        const_reference operator *() const
        {
            return *object;
        }

        void reset(pointer p)
        {
            dec_ref(object);
            object = p;
            inc_ref(object);
        }

        pointer get()
        {
            return object;
        }

        const_pointer get() const
        {
            return object;
        }
            
    private:
        void dec_ref(instrusive_base * p)
        {
            if (p)
            {
                if (p->dec_ref() == 0)
                {
                    delete p;
                }
            }
        }
        void inc_ref(instrusive_base * p)
        {
            if (p)
                p->inc_ref();
        }
        template<typename U>
        friend class instrusive_ptr;
        mutable T *object;
    };
}
