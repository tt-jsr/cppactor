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

#include <mutex>
#include <thread>
#include <atomic>
#include <memory>
#include "cppactor/actor.h"
#include "miscutils/LockFreeMultiProducerQueue.h"
#include <cassert>
#include "cppactor/instrusive_ptr.h"
#include <mutex>
#include <condition_variable>
#include <vector>

namespace cppactor
{
    class actor;
    typedef instrusive_ptr<actor> actor_iptr;

    namespace detail
    {
        class pool_base : public instrusive_base
        {
        public:
            pool_base(uint32_t poolid);

            virtual ~pool_base() {}

            // for actors to notify of new inbound work
            void notify_one(cppactor::actor_iptr actor);
            void notify_one();

            // for actor threads to requeue actors with work still to do
            void renotify_from_the_worker_thread(cppactor::actor_iptr actor);
            void wait_quit();

            uint32_t get_poolid() const {return m_pool_id;}
        protected:
            volatile bool m_quit;
            uint32_t m_pool_id;
            std::mutex m_lockJobsList;
            std::condition_variable m_notify_job;
            miscutils::LowLockMultiProducerQueue<cppactor::actor_iptr> m_actorsWaitingForWork;
            std::vector<std::unique_ptr<std::thread> > m_workers;
        private:
        };

        typedef instrusive_ptr<pool_base> pool_t;
    }
} // cppactor
