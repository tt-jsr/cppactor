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
#include <unordered_map>
#include <mutex>
#include <utility>
#include <iostream>
#include "cppactor/instrusive_ptr.h"
#include "cppactor/timer.h"

namespace cppactor
{
    namespace detail
    {
        class pool_base;
        typedef instrusive_ptr<pool_base> pool_t;
    }
    class actor;
    typedef instrusive_ptr<actor> actor_iptr;

    class framework
    {
    public:
        framework();
        // Get an instance of a framework
        static framework * instance();
        
        void add_actor(actor_iptr a);

        // Stop an actor and release all framework references to it.
        // This will immediately stop the actor, any messages on the queue will
        // be dropped, The actor's on_exit() method will be called. 
        // This actor will not be destroyed if the application is holding any 
        // references to it.
        void stop_actor(actor_iptr actor);

        // Shutdown all thread pools. This is a synchronous call.
        // Each actor's on_exit() will be called.
        void shutdown();

        // Timer for non-actor receivers. 
        // This will be called within the context of the timer thread.
        // 'period' parameter is in milliseconds and should be
        // a multiple of 10 milliseconds.
        // Returns a timer id, the function is passed the timer id
        int set_timer(int period, bool repeat, std::function<void(int)>);

        // Timer for actors. The actor's on_timer(int timerid) will be called within the context of the
        // actor's thread.
        // 'period' parameter is in milliseconds and should be
        // a multiple of 10 milliseconds.
        // Returns a timer id
        int set_timer(actor_iptr actor, int period, bool repeat);

        // Cancel a timer. The timerid was returned by one of the set_timer() functions
        void cancel_timer(int timerid);

        // Get an actor given the actor id
        actor_iptr get_actor(uint32_t actorid);
    private:
        template <typename...ActorTypes>
        friend void create_pool(uint32_t poolid, int nThreads);

        template <typename Actor, typename...Args>
        friend instrusive_ptr<Actor> create_actor(uint32_t poolid, Args&&... args);

        void add_pool(detail::pool_t p);
        detail::pool_t get_pool(uint32_t poolid);
    private:
        static framework *theObject;
        std::unordered_map<uint32_t, actor_iptr > m_actors;
        std::unordered_map<uint32_t, detail::pool_t > m_pools;
        std::mutex m_mtx;
        uint32_t m_timerActorId;
        std::atomic<int> m_timeridpool;
    };
}



