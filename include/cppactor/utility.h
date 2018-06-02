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

#include "cppactor/message.h"
#include "cppactor/actor.h"
#include "cppactor/detail/pool_base.h"
#include "cppactor/detail/pool.h"
#include "cppactor/framework.h"
#include <cassert>

namespace cppactor
{
    enum {POOLID_INTERNAL=1<<31};

/*************************************************************************************/
// Create a pool
template <typename...ActorTypes>
void create_pool(uint32_t poolid, int nThreads)
{
    auto pPool = new detail::pool<ActorTypes...>(poolid);
    pPool->start_threads(nThreads);
    detail::pool_t p(pPool);
    framework::instance()->add_pool(p);
}

/*************************************************************************************/
// Create an actor
template <typename Actor, typename...Args>
instrusive_ptr<Actor> create_actor(uint32_t poolid, Args&&... args)
{
    Actor *t =  new Actor(std::forward<Args>(args)...);
    t->type_id = typeid(Actor).hash_code();
    t->m_pPool = framework::instance()->get_pool(poolid);
    if (t->m_pPool.get() == nullptr)
    {
        assert(false);
        delete t;
        return instrusive_ptr<Actor>();
    }
    t->actor_id = actor::m_actorids.fetch_add(1);

    instrusive_ptr<Actor> p(t);
    framework::instance()->add_actor(p);
    p->on_start();
    return p;
}

/*************************************************************************************/
template<typename T>
auto get_actor(T& t)->decltype(t)
{
    return t;
}

template <typename K, typename V>
auto get_actor(std::pair<K, V>& p)->decltype(p.second)
{
   return p.second;
}

/*********************************************/
template<typename T>
struct value_type;

template<typename K, typename V>
struct value_type<std::pair<K, V> >
{
    typedef V value;  // type V is always a instrusive_ptr
};

template<typename T>
struct value_type
{
    typedef T value;  // type T is always a instrusive_ptr
};


/*
 * Return an actor with the least number of messages on the queue
 * Actor: The type of actor, this is the return type of the function. By default this is cppactor::actor
 * ActorCont: A stl container of actors, one of which will be returned
 *      
 * Example:
 *      std::vector<MyActor> vec;
 *
 *      find_any(vec)->enqueue(new MyMessage(...));
 *          Notice the actor type is not specified since we are enqueuing the message
 *
 *      find_any<MyActor>(vec)->Foo(...)
 *          By specifying the return type, you can call a member function
 */
template<typename Actor=actor, typename ActorCont>
auto find_any(ActorCont& actors)->Actor *
{
    int min = std::numeric_limits<int>::max();
    typename ActorCont::iterator itMin = actors.end();
    for (auto it = actors.begin(); it != actors.end(); ++it)
    {
        auto pActor = get_actor(*it);
        int n = pActor->m_queue.size();
        if (n == 0)
        {
            return static_cast<Actor *>(pActor.get());
        }
        if (n < min)
        {
            itMin = it;
            min = n;
        }
    }
    assert(itMin != actors.end());
    return static_cast<Actor *>(get_actor(*itMin).get());
}

/********************************************************************
 * Broadcast a message to all actors in the container.
 *
 * The arguments are not copied. If you pass a pointer, all actors will
 * share the same pointer
 *
 * MessageType: The type of message to be sent.
 * ActorCont: an stl container containing the actors to broadcast to.
 * Args: variable arguments passed to the constructor of MessageType
 * Example:
 *      std::vector<MyActor> vec;
 *      class MyMessage : public cppactor message {}
 *
 *      broadcast<MyMessage>(vec, arg1, arg2, ...)
 */
template <typename MessageType, typename ActorCont, typename... Args>
void broadcast(ActorCont& actors, Args&&...args)
{
    for (auto it = actors.begin(); it != actors.end(); ++it)
    {
        actor_iptr pActor = get_actor(*it);
        pActor->enqueue(new MessageType(std::forward<Args>(args)...));
    }
}

/********************************************************************
 * Broadcast a function to all actors in the container.
 *
 * ActorCont: an stl container containing the actors to broadcast to.
 * Example:
 *      std::vector<MyActor> vec;
 *
 *      broadcast_call(vec, [=](cppactor::actor_iptr a) {
 *          a.cast<MyActor>()->foo();
 *      });
 */
template <typename ActorCont>
void broadcast_call(ActorCont& actors, std::function<void(cppactor::actor_iptr)>&& f)
{
    for (auto it = actors.begin(); it != actors.end(); ++it)
    {
        actor_iptr pActor = get_actor(*it);
        pActor->enqueue(std::forward<std::function<void(cppactor::actor_iptr)>>(f));
    }
}

/******************************************************************
 * Helper to dispatch a message to an overloaded on_message() function
 *
 * The framework will call your actor's 
 * on_message(cppactor::message_uptr& msg, cppactor::actor_iptr& replyto)
 * When a message is to be process by the actor. You have a choice of 
 * using a switch statement on the msg.msg_id to determine the message type 
 * and cast or use the Dispatch helper
 *
 * Example:
 * void on_message(cppactor::message_uptr& msg, cppactor::actor_iptr& replyto)
 * {
 *      Dispatch<MyMessage1,MyMessage2,MyMessage3>::on_message(this, msg, replyto);
 * }
 *
 * This will invoke one of 
 *    void on_message(std::unique_ptr<MyMessage1>& msg, actor_iptr& replyto)
 *    void on_message(std::unique_ptr<MyMessage2>& msg, actor_iptr& replyto)
 *    void on_message(std::unique_ptr<MyMessage3>& msg, actor_iptr& replyto)
 */
template<typename...Typelist>
struct Dispatch;

template<typename MsgType, typename...Args>
struct Dispatch<MsgType, Args...>
{
    template<typename Actor>
    static void on_message(Actor *actor, cppactor::message_uptr& msg, cppactor::actor_iptr& replyto)
    {
        if (msg->msg_id == MsgType::msg_id)
        {
            std::unique_ptr<MsgType> pmsg(static_cast<MsgType *>(msg.release()));
            actor->on_message(pmsg, replyto);
        }
        else
        {
            Dispatch<Args...>::on_message(actor, msg, replyto);
        }
    }
};

template<> 
struct Dispatch<>  
{
    template<typename Actor>
    inline static void on_message(Actor *, cppactor::message_uptr& msg, cppactor::actor_iptr&) 
    {
        std::cout << "Unhandled message, msg_id=" << msg->msg_id << std::endl;
    }
};

} // cppactor
