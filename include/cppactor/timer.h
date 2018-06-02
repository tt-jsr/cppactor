#pragma once
#include <functional>
#include "cppactor/detail/system_messages.h"
#include "cppactor/message.h"

namespace cppactor
{
    class framework;

    namespace detail
    {
        class timer_actor;
    }

    // Derive from this class when setting a timer
    // for non-actor receivers
    class timer_callback : public message
    {
    public:
        timer_callback(int milliseconds, bool repeat, std::function<void(int)> f)
        :message(detail::timer_message_set_timer)
        , m_timerid(0)
        , m_milliseconds(milliseconds)
        , m_repeat(repeat)
        , func(f)
        {}

        void on_timer()
        {
            func(m_timerid);
        }
    protected:
        friend class detail::timer_actor;
        friend class framework;
        int m_timerid;
        int m_milliseconds;
        bool m_repeat;
        std::function<void(int)> func;
    };

}
