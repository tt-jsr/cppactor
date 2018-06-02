#include <string>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <typeinfo>
#include "cppactor/framework.h"
#include "cppactor/actor.h"
#include "cppactor/message.h"
#include "cppactor/utility.h"

enum Messages
{
    MESSAGE_START_PING
    , MESSAGE_PING
    , MESSAGE_PONG
    , MESSAGE_TEST
};

struct StartPingMessage : public cppactor::message
{
    enum {msg_id=MESSAGE_START_PING};

    StartPingMessage(cppactor::actor_iptr pingto_, const std::string& pingmsg_)
    :cppactor::message(msg_id)
    , pingto(pingto_)
    , pingmsg(pingmsg_)
    {}

    cppactor::actor_iptr pingto;
    std::string pingmsg;
};

struct Ping : public cppactor::message
{
    enum {msg_id=MESSAGE_PING};

    Ping(const std::string& s, cppactor::actor_iptr& replyto)
    :cppactor::message(msg_id, replyto)
    , msg(s)
    {}

    std::string msg;
};

struct Pong : public cppactor::message
{
    enum {msg_id=MESSAGE_PONG};

    Pong(const std::string& s)
    :cppactor::message(msg_id)
    , msg(s)
    {}

    std::string msg;
};

struct TestMessage: public cppactor::message
{
    enum {msg_id=MESSAGE_TEST};
    TestMessage(int n_,  const std::string& s)
    :cppactor::message(msg_id)
    , n(n_)
    , msg(s)
    {}

    int n;
    std::string msg;
};

class Actor1 : public cppactor::actor
{
public:
    void StartPing(cppactor::actor_iptr pingto, const char *msg)
    {
        enqueue(new StartPingMessage(pingto, msg));
    }

    void on_message(std::unique_ptr<StartPingMessage>& msg, cppactor::actor_iptr& reply_to)
    {
        cppactor::actor_iptr a = convert_this();
        msg->pingto->enqueue(new Ping(msg->pingmsg, a));
    }

    void on_message(std::unique_ptr<Ping>& msg, cppactor::actor_iptr& reply_to)
    {
        std::cout << "Actor1 got ping: " << msg->msg << std::endl;
        if (reply_to)
            reply_to->enqueue(new Pong("Replying to " + msg->msg));
    }
            
    void on_message(std::unique_ptr<Pong>& msg, cppactor::actor_iptr& reply_to)
    {
        std::cout << "Actor1 got pong: " << msg->msg << std::endl;
    }

    void on_message(std::unique_ptr<TestMessage>& msg, cppactor::actor_iptr& reply_to)
    {
        std::cout << "Actor1 " << ", " << msg->n << ":" << msg->msg << std::endl;
    }

    void on_message(cppactor::message_uptr& msg, cppactor::actor_iptr replyto)
    {
        cppactor::Dispatch<StartPingMessage,Ping,Pong,TestMessage>::on_message(this, msg, replyto);
    }
};

class Actor2 : public cppactor::actor
{
public:
    void StartPing(cppactor::actor_iptr pingto, const char *msg)
    {
        enqueue(new StartPingMessage(pingto, msg));

    }

    void on_start()
    {
        // Start a timer
        m_timerid = cppactor::framework::instance()->set_timer(convert_this(), 5000, true);
    }
    void on_timer(int timerid)
    {
        assert(timerid == m_timerid);
        std::cout << "Actor2 timer" << std::endl;
    }

    void on_message(std::unique_ptr<StartPingMessage>& msg, cppactor::actor_iptr& replyto)
    {
        cppactor::actor_iptr a = convert_this();
        msg->pingto->enqueue(new Ping(msg->pingmsg, a));
    }

    void on_message(std::unique_ptr<Ping>& msg, cppactor::actor_iptr& replyto)
    {
        std::cout << "Actor2 got ping: " << msg->msg << std::endl;
        if (replyto)
            replyto->enqueue(new Pong("Replying to " + msg->msg));
    }

    void on_message(std::unique_ptr<Pong>& msg, cppactor::actor_iptr& replyto)
    {
        std::cout << "Actor2 got pong: " << msg->msg << std::endl;
    }

    void on_message(cppactor::message_uptr& msg, cppactor::actor_iptr& replyto)
    {
        cppactor::Dispatch<StartPingMessage,Ping,Pong>::on_message(this, msg, replyto);
    }
    int m_timerid;
};

class LongRunningActor : public cppactor::actor
{
public:
    LongRunningActor(const char* name)
    :m_name(name)
    {}

    void Test(int n, const std::string& s)
    {
        enqueue(new TestMessage(n, s));
    }


    void on_message(std::unique_ptr<TestMessage>& msg, cppactor::actor_iptr reply_to)
    {
        std::this_thread::sleep_for(std::chrono::seconds(msg->n));
        std::cout << m_name << ", " << msg->n << ":" << msg->msg << std::endl;
    }

    void on_message(cppactor::message_uptr& msg, cppactor::actor_iptr reply_to)
    {
        cppactor::Dispatch<TestMessage>::on_message(this, msg, reply_to);
    }
    std::string m_name;
};

enum POOLIDS
{
    POOLID_INVALID = 0
    , POOLID_QUICK = 1
    , POOLID_LONGRUNNING = 2
};

/*************************************
 * Run some tests
 * We create two pools, one for processing actors that handle messages quickly(Actor1, Actor2), another pool
 * for long running actors that remain blocked for long times (to simulate web requests)
 *
 * This demonstrates how to send a message to a particular actor, as well has having the framework pick the actor
 * to peform the work.
 */

void foo(int)
{
    std::cout << "foo called" << std::endl;
}

int main(int argc, char *argv[])
{
    // Create the framework
    cppactor::framework framework;

    // Create two thread pools
    // The template arguments define the types of actors this pool will process, each pool requires
    // a application defined pool id, and the number of threads to create
    cppactor::create_pool<Actor1, Actor2>(POOLID_QUICK, 3);
    cppactor::create_pool<LongRunningActor>(POOLID_LONGRUNNING, 3);

    std::vector<cppactor::actor_iptr> longrunningActors;
    // Create our actors
    // The create_actor<>() function requires the type of actor to create, and the pool id this actor will be 
    // associated with
    cppactor::instrusive_ptr<Actor1> a1 = cppactor::create_actor<Actor1>(POOLID_QUICK);
    cppactor::instrusive_ptr<Actor2> a2 = cppactor::create_actor<Actor2>(POOLID_QUICK);
    cppactor::instrusive_ptr<LongRunningActor> l1 = cppactor::create_actor<LongRunningActor>(POOLID_LONGRUNNING, "lra 1");
    cppactor::instrusive_ptr<LongRunningActor> l2 = cppactor::create_actor<LongRunningActor>(POOLID_LONGRUNNING, "lra 2");

    longrunningActors.push_back(l1);
    longrunningActors.push_back(l2);

    a1->enqueue([=](cppactor::actor_iptr a) {std::cout << "Function object called" << std::endl;});

    // Start a timer
    int tid = framework.set_timer(5000, true, [=](int timer_id) {
        std::cout << "on_timer" << std::endl;
    });

    std::this_thread::sleep_for(std::chrono::seconds(20));
    std::cout << "Canceling timer" << std::endl;
    framework.cancel_timer(tid);
    std::this_thread::sleep_for(std::chrono::seconds(30));

    // have actor1 ping actor2
    a1->StartPing(a2, "Ping msg");

    // Send a message to a particular LongRunningActor using the enqueue interface
    l1->enqueue(new TestMessage(5, "Wait 5 sec, I chose actor"));

    // Have the framework pick an actor (based on queue length)
    cppactor::find_any(longrunningActors)->enqueue(new TestMessage(5, "Wait 5 sec, framework chose actor"));

    // Send a message to a particular LongRunningActor using an actor interface
    l1->Test(5, "Wait 5 sec, I chose actor interface");

    // Have the framework pick an actor (based on queue length), using actor's interface
    cppactor::find_any<LongRunningActor>(longrunningActors)->Test(5, "Wait 5 sec, framework chose actor interface");
    
    // Broadcast to actors
    cppactor::broadcast<TestMessage>(longrunningActors, 5, "Wait 5 sec, broadcast");

    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Test actor stop functionality
       // pump a bunch of messages
    std::cout << "Test actor stop" << std::endl;
    for (int i = 0; i < 1000; i++)
    {
        std::cout << "Enqueue " << i << ":" << "test" << std::endl;
        a1->enqueue(new TestMessage(i, "test"));
    }
    //framework.stop_actor(a1);

    std::this_thread::sleep_for(std::chrono::seconds(10));
    framework.shutdown();

    return 0;
}


