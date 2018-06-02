#include "cppactor/actor.h"
#include "cppactor/detail/pool_base.h"
#include "cppactor/message.h"
#include "cppactor/detail/system_messages.h"

namespace cppactor
{
    std::atomic<uint32_t> actor::m_actorids(1);

    actor::~actor()
    {
        miscutils::SpinLockMonitor<miscutils::SimpleSpinLock> lock(m_spin_lock);

        while(!m_queue.empty())
        {
            delete m_queue.front();
            m_queue.pop();
        }
    }

    unsigned int actor::enqueue(std::function<void (cppactor::actor_iptr)>&& f)
    {
        detail::function_invoke_msg *pMsg = new detail::function_invoke_msg(std::forward<std::function<void(cppactor::actor_iptr)>>(f));
        return enqueue(pMsg);
    }

}
