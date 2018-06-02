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

#include <string>
#include <iostream>
#include <vector>
#include <typeinfo>
#include <mutex>
#include <thread>
#include <atomic>
#include <memory>
#include <limits>
#include <condition_variable>
#include "cppactor/message.h"
#include "cppactor/actor.h"
#include "cppactor/detail/pool_base.h"
#include "cppactor/framework.h"
#include "cppactor/detail/system_messages.h"
#include "miscutils/LockFreeMultiProducerQueue.h"
#include <cassert>
#include "logger/logger.h"

namespace cppactor
{
    namespace detail
    {
        template<typename...Typelist>
        struct on_message_helper;

        template<typename ActorType, typename...Typelist>
        struct on_message_helper<ActorType, Typelist...>
        {
            inline static void on_message(actor_iptr ab, std::unique_ptr<cppactor::message>& msg)
            {
                if (ab->type_id == typeid(ActorType).hash_code())
                {
                    cppactor::actor_iptr a = msg->get_reply_to();
                    ab.cast<ActorType>()->on_message(msg, a);
                }
                else
                {
                    on_message_helper<Typelist...>::on_message(ab, msg);
                }
            }
        };

        template<> 
        struct on_message_helper<>  
        {
            inline static void on_message(actor_iptr, std::unique_ptr<cppactor::message>&) {}
        };

        /**************************************************************************************/

        template <typename...Typelist>
        class pool : public pool_base
        {
        public:
            pool(uint32_t poolid);

            pool(const pool&) = delete;
            pool(pool&&) = delete;
            pool& operator = (const pool&) = delete;
            pool& operator = (pool&&) = delete;

            ~pool();

            void start_threads(int numThreads);

        private:
            void thread_worker();
        };


        template <typename...Typelist>
        pool<Typelist...>::pool(uint32_t poolid)
        :pool_base(poolid)
        {
        }

        template <typename...Typelist>
        pool<Typelist...>::~pool()
        {

        }

        template <typename...Typelist>
        void pool<Typelist...>::start_threads(int numThreads)
        {
            for (int i = 0; i < numThreads; ++i)
            {
                m_workers.emplace_back(new std::thread(&pool::thread_worker, this));
            }
        }

        template <typename...Typelist>
        void pool<Typelist...>::thread_worker()
        {
            try
            {
                while (!m_quit)
                {
                    actor_iptr ab;
                    bool haveItem=false;
                    if (!m_actorsWaitingForWork.Consume(ab))
                    {
                        std::unique_lock<std::mutex> lockList(m_lockJobsList);
                        if (!m_actorsWaitingForWork.Consume(ab))
                            m_notify_job.wait(lockList);
                        else
                            haveItem=true;
                    }
                    else
                        haveItem=true;

                    if (haveItem)
                    {
                        if (ab->is_stopped() == false)
                        {
                            cppactor::message *pMsg;
                            // take one work item and one work item only
                            if (ab->consume_one_item(pMsg))
                            {
                                if (pMsg->msg_id == detail::timer_on_timer::msg_id)
                                {
                                    // looks like a timer message, call on_timer()
                                    detail::timer_on_timer *p = static_cast<detail::timer_on_timer *>(pMsg);
                                    ab->on_timer(p->m_timerid);
                                    delete p;
                                }
                                else if (pMsg->msg_id == detail::function_invoke_msg::msg_id)
                                {
                                    detail::function_invoke_msg *p = static_cast<detail::function_invoke_msg *>(pMsg);
                                    p->m_func(ab);
                                    delete p;
                                }
                                else
                                {
                                    std::unique_ptr<cppactor::message> msg(pMsg);
                                    on_message_helper<Typelist...>::on_message(ab, msg);
                                }
                                // see if there is more work, and should requeue the actor
                                ab->requeue();
                            }
                        }
                        else
                        {
                            // if ab is stopped, it means do nothing more with it (ever). Dont requeue, dont process its messgaes, just loop around for a new actor
                        }
                    }

                }
            }
            catch (std::exception& e)
            {
                assert(false);
                TTLOG( ERROR, 13 )  << "standard exception in cppa pool -  will terminate, what:" << e.what();
                quick_exit(EXIT_FAILURE);
            }
            catch (...)
            {
                assert(false);
                TTLOG( ERROR, 13 )  << "Unknown exception in cppa pool -  will terminate";
                quick_exit(EXIT_FAILURE);
            }
            TTLOG(INFO, 0) << "Pool thread shutdown";
        }

    } // detail
} // cppactor
