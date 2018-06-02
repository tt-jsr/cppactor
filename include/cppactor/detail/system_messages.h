#pragma once
#include "cppactor/message.h"
#include <functional>

namespace cppactor
{
    namespace detail
    {
        enum system_message_types
        {
            system_message_type_none = 0
            , system_message_start = 1 << 31    // not a real message, system message sstart at this id
            , timer_message_invoke              // Internal message to wake up timer actor
            , timer_message_set_timer
            , timer_message_on_timer
            , timer_message_cancel_timer
            , function_message_invoke
        };

        class timer_on_timer : public cppactor::message
        {
        public:
            enum {msg_id = timer_message_on_timer};
            timer_on_timer(int id)
            :cppactor::message(msg_id)
            , m_timerid(id)
            {}

            int m_timerid;
        };   

        class timer_cancel_timer : public cppactor::message
        {
        public:
            enum {msg_id = timer_message_cancel_timer};
            timer_cancel_timer(int id)
            :cppactor::message(msg_id)
            , m_timerid(id)
            {}

            int m_timerid;
        };   

        class function_invoke_msg : public message
        {
        public:
            enum {msg_id = function_message_invoke};
            function_invoke_msg(std::function<void (cppactor::actor_iptr)>&& f)
            : message(msg_id)
            , m_func(std::move(f))
            {}

            std::function<void (cppactor::actor_iptr)> m_func;
        };
    }
}

