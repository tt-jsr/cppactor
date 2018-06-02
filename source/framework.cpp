#include "cppactor/framework.h"
#include "cppactor/detail/pool.h"
#include <utility>
#include "cppactor/timer.h"
#include "cppactor/detail/system_messages.h"
#include "cppactor/detail/timer_actor.h"
#include "cppactor/utility.h"

namespace cppactor
{
    framework *framework::theObject = nullptr;

    framework::framework()
    :m_timerActorId(0)
    , m_timeridpool(0)
    {
        theObject = this;
        // Create the thread for the timer actor
        create_pool<detail::timer_actor>(POOLID_INTERNAL, 1);
        actor_iptr a = create_actor<detail::timer_actor>(POOLID_INTERNAL);
        m_timerActorId = a->get_actorid();
    }

    framework *framework::instance()
    {
        return theObject;
    }
    
    int framework::set_timer(int period, bool repeat, std::function<void(int)> f)
    {
        assert(0 == period % 10);
        int tid = ++m_timeridpool;
        timer_callback *p = new timer_callback(period, repeat, f);
        p->m_timerid = tid;
        actor_iptr t = get_actor(m_timerActorId);
        assert(t);
        t->enqueue(p);
        return tid;
    }

    int framework::set_timer(actor_iptr actor, int period, bool repeat)
    {
        assert(0 == period % 10);

        // We hold on to the actorid instead of the actor itself as a sort of
        // weak reference. We don't want to keep it alive if the actor is stopped.
        uint32_t aid = actor->get_actorid();

        return set_timer(period, repeat, [=](int timer_id) {
            actor_iptr a = framework::instance()->get_actor(aid);
            if (a)
            {
                a->enqueue(new detail::timer_on_timer(timer_id));
            }
        });
    }

    void framework::cancel_timer(int timerid)
    {
        detail::timer_cancel_timer *m = new detail::timer_cancel_timer(timerid);
        actor_iptr t = get_actor(m_timerActorId);
        assert(t);
        t->enqueue(m);
    }

    detail::pool_t framework::get_pool(uint32_t poolid)
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        auto it = m_pools.find(poolid);
        if (it == m_pools.end())
            return detail::pool_t();
        return (*it).second;
    }

    actor_iptr framework::get_actor(uint32_t actorid)
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        auto it = m_actors.find(actorid);
        if (it == m_actors.end())
            return actor_iptr();
        return (*it).second;
    }

    void framework::add_pool(detail::pool_t p)
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_pools.insert(std::make_pair(p->get_poolid(), p));
    }

    void framework::add_actor(actor_iptr a)
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_actors.insert(std::make_pair(a->get_actorid(), a));
    }

    void framework::stop_actor(actor_iptr actor)
    {
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            m_actors.erase(actor->get_actorid());
        }
        actor->on_exit();
        actor->stop();
    }

    void framework::shutdown()
    {
        std::vector<detail::pool_t> pools;
        std::vector<actor_iptr> actors;

        {
            std::lock_guard<std::mutex> lock(m_mtx);
            for (auto it = m_pools.begin(); it != m_pools.end(); ++it)
            {
                pools.push_back((*it).second);
            }
            for (auto ait = m_actors.begin(); ait != m_actors.end(); ++ait)
            {
                actors.push_back((*ait).second);
            }
        }

        for (auto it = pools.begin(); it != pools.end(); ++it)
        {
            (*it)->wait_quit();
        }
        for (auto ait = actors.begin(); ait != actors.end(); ++ait)
        {
            (*ait)->on_exit();
        }
        
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            m_pools.clear();
            m_actors.clear();
        }
    }
}
