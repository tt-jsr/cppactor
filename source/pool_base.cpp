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
#include <memory>
#include "cppactor/actor.h"
#include "miscutils/LockFreeMultiProducerQueue.h"
#include <cassert>
#include "cppactor/instrusive_ptr.h"
#include "cppactor/detail/pool_base.h"

namespace cppactor
{
    namespace detail
    {
        class actor;
        typedef instrusive_ptr<actor> actor_iptr;

        pool_base::pool_base(uint32_t poolid_)
        :m_quit(false)
        ,m_pool_id(poolid_)
        {
        }

        void pool_base::notify_one(cppactor::actor_iptr actor)
        {
            m_actorsWaitingForWork.Produce(actor);
            std::unique_lock<std::mutex> lockList(m_lockJobsList);
            m_notify_job.notify_one();// wake up a thread if any are idle
        }

        void pool_base::notify_one()
        {
            m_notify_job.notify_one();// wake up a thread if any are idle
        }

        void pool_base::wait_quit()
        {
            m_quit = true;
            m_notify_job.notify_all();

            // Wait for all the threads to terminate
            for(std::unique_ptr<std::thread>& worker: m_workers)
            {
                worker->join();
            }
        }
    } // detail
} // cppactor
