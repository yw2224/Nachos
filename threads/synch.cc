// synch.cc
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"
#include "thread.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts

    while (value == 0) { 			// semaphore not available
	      queue->Append((void *)currentThread);	// so go to sleep
        //printf("Sleeping thread %d...\n", currentThread->getThreadID());
	      currentThread->Sleep();
    }
    value--; 					// semaphore available,
						// consume its value

    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)	   // make thread ready, consuming the V immediately
	  {
        scheduler->ReadyToRun(thread);
        //printf("thread %d ready to run!\n", thread->getThreadID());
    }
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments
// Note -- without a correct implementation of Condition::Wait(),
// the test case in the network assignment won't work!
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
Lock::Lock(char* debugName)
{
    name = debugName;
    lockSemaphore = new Semaphore(debugName, 1);
}
Lock::~Lock()
{
    delete lockSemaphore;
}

void Lock::Acquire()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    lockSemaphore->P();
    holdingThread = currentThread;
    (void) interrupt->SetLevel(oldLevel);
}
void Lock::Release()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    ASSERT(currentThread == holdingThread);
    lockSemaphore->V();
    holdingThread = NULL;
    (void) interrupt->SetLevel(oldLevel);
}

bool Lock::isHeldByCurrentThread()
{
    return holdingThread == currentThread;
}

Condition::Condition(char* debugName)
{
    name = debugName;
    waitingList = new List;
}
Condition::~Condition()
{
    delete waitingList;
}

void Condition::Wait(Lock* conditionLock)
{   //ASSERT(FALSE);
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    ASSERT(conditionLock->isHeldByCurrentThread());

    conditionLock->Release();
    waitingList->Append(currentThread);
    currentThread->Sleep();
    conditionLock->Acquire();

    (void) interrupt->SetLevel(oldLevel);
}

void Condition::Signal(Lock* conditionLock)
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    ASSERT(conditionLock->isHeldByCurrentThread());

    Thread* signalThread = (Thread*) waitingList->Remove();
    if(signalThread != NULL)
    {
        scheduler->ReadyToRun(signalThread);
        printf("thread %d ready to run\n", signalThread->getThreadID());
    }

    (void) interrupt->SetLevel(oldLevel);
}

void Condition::Broadcast(Lock* conditionLock)
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    ASSERT(conditionLock->isHeldByCurrentThread());

    Thread* signalThread;
    while(!waitingList->IsEmpty())
    {
        signalThread = (Thread*) waitingList->Remove();
        if(signalThread != NULL)
        {
            scheduler->ReadyToRun(signalThread);
            //printf("thread %d ready to run\n", signalThread->getThreadID());
        }
    }
    (void) interrupt->SetLevel(oldLevel);
}
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
ProducerConsumerCond::ProducerConsumerCond(int capacity)
{
    mutex = new Lock("mutex");
    empty = new Condition("empty");
    full = new Condition("full");
    bufferCapacity = capacity;
}

ProducerConsumerCond::~ProducerConsumerCond()
{
    delete mutex;
    delete empty;
    delete full;
}

void ProducerConsumerCond::Producer(int total)
{
    for(int i = 0; i < total; i ++)
    {
        mutex->Acquire();
        while(buffer == bufferCapacity)
        {
            printf("Buffer full! Sleeping the producer...\n\n");
            full->Wait(mutex);
        }
        buffer ++;
        printf("Producer puts an item in buffer. %d items in total.\n", buffer);
        if(buffer == 1)
        {
            printf("Sending signal waking up the consumer...\n\n");
            empty->Signal(mutex);
        }
        mutex->Release();
    }
}

void ProducerConsumerCond::Consumer(int total)
{
    for(int i = 0; i < total; i ++)
    {
        mutex->Acquire();
        while(buffer == 0)
        {
            printf("Empty buffer! Sleeping the consumer...\n\n");
            empty->Wait(mutex);
        }
        buffer --;
        printf("Consumer consumes an item in buffer. %d items in total.\n", buffer);
        if(buffer == bufferCapacity - 1)
        {
            full->Signal(mutex);
            printf("Sending signal waking up the producer...\n\n");
        }
        mutex->Release();
  }
}
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

ProducerConsumerSem::ProducerConsumerSem(int capacity)
{
    mutex = new Semaphore("mutex", 1);
    empty = new Semaphore("empty", capacity);
    full = new Semaphore("full", 0);
    bufferCapacity = capacity;
}

ProducerConsumerSem::~ProducerConsumerSem()
{
    delete mutex;
    delete empty;
    delete full;
}

void ProducerConsumerSem::Producer(int total)
{
    for(int i = 0; i < total; i ++)
    {
        empty->P();
        mutex->P();
        buffer ++;
        printf("Producer puts an item in buffer. %d items in total.\n", buffer);
        if(buffer == bufferCapacity)
            printf("Buffer full! \n\n");
        mutex->V();
        full->V();
    }
}

void ProducerConsumerSem::Consumer(int total)
{
    for(int i = 0; i < total; i ++)
    {
        full->P();
        mutex->P();
        buffer --;
        printf("Consumer consumes an item in buffer. %d items in total.\n", buffer);
        if(buffer == 0)
            printf("Empty buffer! \n\n");
        mutex->V();
        empty->V();
  }
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

Barrier::Barrier()
{
    mutex = new Lock("mutex");
    allReady = new Condition("allReady");
}

Barrier::~Barrier()
{
    delete mutex;
    delete allReady;
}

void Barrier::MemberThread(int num)
{
    mutex->Acquire();

    printf("thread %d ready\n", currentThread->getThreadID());
    readyNum ++;

    while(readyNum != num)
    {
        printf("thread %d waiting...\n", currentThread->getThreadID());
        allReady->Wait(mutex);
    }
    if(readyNum == num)
    {
        printf("All threads ready! Broadcasting...\n");
        allReady->Broadcast(mutex);
    }

    mutex->Release();

    printf("thread %d continues to run\n", currentThread->getThreadID());
}

ReaderWriter::ReaderWriter()
{
    mutex = new Lock("mutex");
    buffer = new Lock("buffer");
}

ReaderWriter::~ReaderWriter()
{
    delete mutex;
    delete buffer;
}

void ReaderWriter::Reader()
{
    mutex->Acquire();
    readerNum ++;
    if(readerNum == 1)
        buffer->Acquire();
    mutex->Release();

    printf("%s is reading...\n", currentThread->getName());
    //printf("%d reader in total\n", readerNum);
    interrupt->OneTick();

    mutex->Acquire();
    readerNum --;
    printf("%s finishes reading!\n", currentThread->getName());
    if(readerNum == 0)
    {
        buffer->Release();
    }
    mutex->Release();
}

void ReaderWriter::Writer()
{
    buffer->Acquire();
    printf("%s is writing...\n", currentThread->getName());
    printf("%s finishes writing!\n", currentThread->getName());
    buffer->Release();

}
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
