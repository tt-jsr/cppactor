#pragma once

#include <chrono>
#include <thread>
#include "cppactor/actor.h"
#include "cppactor/message.h"
#include "cppactor/detail/system_messages.h"
#include "cppactor/timer.h"
#include <iostream>
#include <memory>
#include <ctime>
#include <utility>
#include <map>
#include <set>
#include <cassert>
#include <chrono>

using namespace std::chrono;

using std::cout; 
using std::endl;

namespace cppactor
{
    namespace detail
    {

        class timer_actor : public actor
        {
        public:
            timer_actor()
            {
            }

            void on_start()
            {
                //TTLOG(INFO, 0) << "CPPACTOR | Timer actor started";
                this->enqueue(new message(timer_message_invoke));
            }

            void add_timer(const high_resolution_clock::time_point& t, timer_callback *cb)
            {
                //TTLOG(INFO, 0) << "CPPACTOR | Adding timer tid: " << cb->m_timerid;
                auto it = m_timers.find(t);
                if (it == m_timers.end())
                {
                    std::set<timer_callback *> set_;
                    set_.insert(cb);
                    m_timers.insert(std::make_pair(t, std::move(set_)));
                }
                else
                {
                    it->second.insert(cb);
                }
            }

            void cancel_timer(int timerid)
            {
                // do this the hard way
                for (auto it = m_timers.begin(); it != m_timers.end(); ++it)
                {
                    std::set<timer_callback *>& set_ = it->second;
                    for (auto sit = set_.begin(); sit != set_.end(); ++sit)
                    {
                        if ((*sit)->m_timerid == timerid)
                        {
                            set_.erase(sit);
                            if (set_.size() == 0)
                            {
                                m_timers.erase(it);
                            }
                            return;
                        }
                    }
                }
            }

            void on_message(std::unique_ptr<message>& msg, actor_iptr reply_to)
            {
                switch (msg->msg_id)
                {
                    case timer_message_invoke:
                    {
                        high_resolution_clock::time_point now = high_resolution_clock::now();
                        auto it = m_timers.begin();
                        while (it != m_timers.end() && it->first <= now )
                        {
                            std::set<timer_callback *>& set_ = it->second;
                            for(timer_callback *cb : set_)
                            {
                                //TTLOG(INFO, 0) << "CPPACTOR | Timer callback tid: " << cb->m_timerid;
                                cb->on_timer();
                                if (cb->m_repeat)
                                {
                                    add_timer(now + duration<int, std::milli>(cb->m_milliseconds), cb);
                                }
                            }
                            m_timers.erase(it++);
                        }

                        //std::cout << "Timer" << std::endl;
                        message *pMsg = msg.release();

                        std::this_thread::sleep_for(milliseconds(10));
                        this->enqueue(pMsg); // post the message again to keep us going
                        break;
                    }
                    case timer_message_set_timer:
                    {
                        timer_callback * cb(static_cast<timer_callback *>(msg.release()));
                        high_resolution_clock::time_point t = high_resolution_clock::now() + duration<int, std::milli>(cb->m_milliseconds);
                        add_timer(t, cb);
                        break;
                    }
                    case timer_message_cancel_timer:
                    {
                        timer_cancel_timer * ct(static_cast<timer_cancel_timer *>(msg.get()));
                        cancel_timer(ct->m_timerid);
                    }
                }
            }
            std::map<high_resolution_clock::time_point, std::set<timer_callback *> > m_timers;
        };
    }
}

