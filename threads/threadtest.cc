// threadtest.cc
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield,
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "interrupt.h"
#include "unistd.h"
#include "scheduler.h"
#include "synch.h"

// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void
SimpleThread(int which)
{
    int num;
    for (num = 0; num < 3; num++)
    {
        //printf("*** thread %d looped %d times. userID = %d, threadID = %d, priority = %d\n", which, num, currentThread->getUserID(), currentThread->getThreadID(), currentThread->getPriority());
        //currentThread->MultiQueueYield(currentThread->getPriority());
        currentThread->Yield();
        interrupt->OneTick();
        //interrupt->SetLevel(IntOn);
        //interrupt->SetLevel(IntOff);
        //Thread *t = new Thread("forked thread-2", 2);
        //t->Fork(p2, 2);
    }
}





//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = new Thread("forked thread");
    t->Fork(SimpleThread, 1);

    SimpleThread(0);
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

//interrupt->SetLevel(IntOn);
//interrupt->SetLevel(IntOff);
//        interrupt->SetLevel(IntOn);
//interrupt->SetLevel(IntOff);
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/



void PriorityTest2(int which)
{
    int num;
    for (num = 0; num < 12; num++)
    {
        //printf("*** thread %d looped %d times. userID = %d, threadID = %d, priority = %d\n", which, num, currentThread->getUserID(), currentThread->getThreadID(), currentThread->getPriority());
        interrupt->OneTick();
        if(num == 2)
        {
            Thread *t3 = new Thread("forked thread-3");
            t3->MultiQueueFork(SimpleThread, t3->getThreadID());
        }
    }
}


void PriorityTest1(int which)
{
    int num;
    for (num = 0; num < 27; num++)
    {
        //printf("*** thread %d looped %d times. userID = %d, threadID = %d, priority = %d\n", which, num, currentThread->getUserID(), currentThread->getThreadID(), currentThread->getPriority());
        interrupt->OneTick();
        if(num == 4)
        {
            Thread *t2 = new Thread("forked thread-2");
            t2->MultiQueueFork(PriorityTest2, t2->getThreadID());
        }
    }
}

void MultiQueueThreadTest()
{
    Thread *t1 = new Thread("forked thread-1");

    t1->MultiQueueFork(PriorityTest1, t1->getThreadID());
}


void ThreadTest4()
{
    DEBUG('t', "Entering ThreadTest4");

    Thread *t1 = new Thread("forked thread-1");
    Thread *t2 = new Thread("forked thread-2");
    Thread *t3 = new Thread("forked thread-3");
    t1->setPriority(7);
    t2->setPriority(4);
    t3->setPriority(5);

    t1->Fork(SimpleThread, t1->getThreadID());
    t2->Fork(SimpleThread, t2->getThreadID());
    t3->Fork(SimpleThread, t3->getThreadID());

    //SimpleThread(0);
}
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
//Fork只是把它们放到readyList，而当前main已经在运行了
void ThreadTest128()
{
    DEBUG('t', "Entering ThreadTest128");

    for(int i = 0; i < MAX_THREAD; i ++)
    {
        Thread *t = new Thread("forked thread");
        printf("*** thread %s, userID = %d, threadID = %d\n", t->getName(), t->getUserID(), t->getThreadID());
    }
}

void SystemThreadsPrint(int arg)
{
    for(int i = 0; i < MAX_THREAD; i ++)
    {
        if(systemThreads[i].getAllocated() == 1)
        {
            printf("*** thread %s, userID = %d, threadID = %d, status = %s\n", systemThreads[i].getThreadPtr()->getName(), systemThreads[i].getThreadPtr()->getUserID(), systemThreads[i].getThreadPtr()->getThreadID(), systemThreads[i].getThreadPtr()->getThreadStatus());
        }
    }
}

void ThreadPrintTest()
{
    Thread *t1 = new Thread("forked thread");
    Thread *t2 = new Thread("forked thread");
    Thread *t3 = new Thread("forked thread");

    t1->Fork(SystemThreadsPrint, 0);

}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*
Lock *mutex;
Condition *empty;
Condition *full;
int buffer;

void Producer(int total)
{
    for(int i = 0; i < total; i ++)
    {
        mutex->Acquire();
        while(buffer == 5)
        {
            printf("buffer full! Waking up consumer...\n");
            full->Wait(mutex);
        }
        buffer ++;
        printf("Producer puts an item in buffer. %d items in total.\n", buffer);
        empty->Signal(mutex);
        mutex->Release();
    }
}

void Consumer(int total)
{
    for(int i = 0; i < total; i ++)
    {
        mutex->Acquire();
        while(buffer == 0)
        {
            printf("empty buffer! Waking up producer...\n");
            empty->Wait(mutex);
        }
        buffer --;
        printf("Consumer consumes an item in buffer. %d items in total.\n", buffer);
        full->Signal(mutex);
        mutex->Release();
  }
}
*/

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
ProducerConsumerCond *cond = new ProducerConsumerCond(5);
ProducerConsumerSem *sem = new ProducerConsumerSem(5);
Barrier *barrier = new Barrier();
ReaderWriter *readerwriter = new ReaderWriter();

void ProducerMediate(int total)
{
    sem->Producer(total);
}

void ConsumerMediate(int total)
{
    sem->Consumer(total);
}

void barrierMediate(int num)
{
    barrier->MemberThread(num);
}

void ReaderMediate(int which)
{
    for(int i = 0; i < which; i ++)
        readerwriter->Reader();
}

void WriterMediate(int which)
{
    for(int i = 0; i < which; i ++)
        readerwriter->Writer();
}

void
ThreadSynchTest()
{

  /*  Thread *t1 = new Thread("producer");
    Thread *t2 = new Thread("consumer");
    t1->setPriority(1);
    t2->setPriority(1);

    t1->Fork(ProducerMediate, 8);
    t2->Fork(ConsumerMediate, 8);*/

  /*  Thread *t1 = new Thread("thread1");
    Thread *t2 = new Thread("thread2");
    Thread *t3 = new Thread("thread3");
    t1->setPriority(1);
    t2->setPriority(1);
    t3->setPriority(1);

    t1->Fork(barrierMediate, 3);
    t2->Fork(barrierMediate, 3);
    t3->Fork(barrierMediate, 3);*/
    Thread *t1 = new Thread("reader 1");
    Thread *t2 = new Thread("reader 2");
    Thread *t3 = new Thread("reader 3");
    Thread *t4 = new Thread("writer 1");
    Thread *t5 = new Thread("writer 2");
    t1->setPriority(5);
    t2->setPriority(5);
    t3->setPriority(5);
    t4->setPriority(5);
    t5->setPriority(5);

    t4->Fork(WriterMediate, 3);
    t5->Fork(WriterMediate, 3);
    t1->Fork(ReaderMediate, 3);
    t2->Fork(ReaderMediate, 3);
    t3->Fork(ReaderMediate, 3);

}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void ThreadTest()
{
    switch (testnum)
    {
    case 1:
        ThreadTest1();
        break;
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    case 0:
        ThreadPrintTest();
        break;
    case 2:
        MultiQueueThreadTest();
        break;
    case 3:
        ThreadSynchTest();
        break;
    case 4:
        ThreadTest4();
        break;
    case 128:
        ThreadTest128();
        break;
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    default:
        printf("No test specified.\n");
	break;
    }
}
