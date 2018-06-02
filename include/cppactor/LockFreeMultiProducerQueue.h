/***************************************************************************
 *
 *                    Unpublished Work Copyright (c) 2012, 2014
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
// This is based on the Sutter MPMC queue detailed in Dr Dobbs Journal
// http://www.drdobbs.com/parallel/writing-a-generalized-concurrent-queue/211601363
// It is kernel lock free, but not strictly lock free, as it relies on atomics and spinning on them, and can do so efficiently
// because the duration an atomic is 'held' is entirely within this code (no appl callbacks) and is a guaranteed small # of ops.

#pragma once

#ifndef LOCKFREEMULTIPRODUCERQUEUE_H
#define LOCKFREEMULTIPRODUCERQUEUE_H

#include <atomic>
#include <ttstl/platform.h>

namespace miscutils
{

template <typename T>
struct LowLockMultiProducerQueue
{
private:
	struct Node {
		Node( T* val ) : value(val), next(nullptr) { }
	    T* value;
	    std::atomic<Node*> next;
	    char pad[TT_CACHE_LINE_SIZE - sizeof(T*)- sizeof(std::atomic<Node*>)];
	};

    char pad0[TT_CACHE_LINE_SIZE];

	// for one consumer at a time
	Node* first;

	char pad1[TT_CACHE_LINE_SIZE
	         - sizeof(Node*)];

	// shared among consumers
	std::atomic<bool> consumerLock;

	char pad2[TT_CACHE_LINE_SIZE
	         - sizeof(std::atomic<bool>)];

	// for one producer at a time
	Node* last;

	char pad3[TT_CACHE_LINE_SIZE
	         - sizeof(Node*)];

	// shared among producers
	std::atomic<bool> producerLock;

	char pad4[TT_CACHE_LINE_SIZE
	         - sizeof(std::atomic<bool>)];

public:
	LowLockMultiProducerQueue()
	{
	    first = last = new Node( nullptr );
    	producerLock = consumerLock = false;
  	}
  	~LowLockMultiProducerQueue()
  	{
    	while( first != nullptr )
    	{      // release the list
      		Node* tmp = first;
      		first = tmp->next;
      		delete tmp->value;       // no-op if null
      		delete tmp;
    	}
  	}

  	void Produce( const T& t )
  	{
  		Node* tmp = new Node( new T(t) );
  		while( producerLock.exchange(true) )
    		{ }   // acquire exclusivity
  		last->next = tmp;         // publish to consumers
  		last = tmp;             // swing last forward
  		producerLock = false;       // release exclusivity
	}

	bool Consume( T& result )
	{
		while( consumerLock.exchange(true) )
	    	{ }    // acquire exclusivity

	    Node* theFirst = first;
		Node* theNext = first-> next;
		if( theNext != nullptr ) {   // if queue is nonempty
	  		T* val = theNext->value;    // take it out
	  		theNext->value = nullptr;  // of the Node
	  		first = theNext;          // swing first forward
	  		consumerLock = false;             // release exclusivity

	  		result = *val;    // now copy it back
	  		delete val;       // clean up the value
	  		delete theFirst;      // and the old dummy
	  		return true;      // and report success
		}
		consumerLock = false;   // release exclusivity
    	return false;                  // report queue was empty
	}
};

}   //  namespace miscutils

#endif  //  LOCKFREEMULTIPRODUCERQUEUE_H

