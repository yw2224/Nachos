// progtest.cc
//	Test routines for demonstrating that Nachos can load
//	a user program and execute it.
//
//	Also, routines for testing the Console hardware device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "console.h"
#include "addrspace.h"
#include "synch.h"
#include "synchconsole.h"

//----------------------------------------------------------------------
// StartProcess
// 	Run a user program.  Open the executable, load it into
//	memory, and jump to it.
//----------------------------------------------------------------------
void _ForkThread(int i) {

    printf("Starting second thread\n\n");

    if (currentThread->space != NULL) {		// if there is an address space
          currentThread->RestoreUserState();     // to restore, do it.
    	    currentThread->space->RestoreState();
    }

    //_space->InitRegisters();		// set the initial register values
    //_space->RestoreState();
    machine->Run();
}

//StartProcess for exercise 5, 2 threads
// void
// StartProcess(char *filename)
// {
//     OpenFile *executable = fileSystem->Open(filename);
//     AddrSpace *space;
//
//     OpenFile *_executable = fileSystem->Open("../test/halt");
//     AddrSpace *_space;
//     Thread *_thread = new Thread("_thread");
//
//     if (executable == NULL) {
// 	      printf("Unable to open file %s\n", filename);
// 	      return;
//     }
//
//     space = new AddrSpace(executable, "disk");
//     _space = new AddrSpace(_executable, "halt") ;
//     currentThread->space = space;
//
//     currentThread->userProgFilename = "disk";
//     _thread->userProgFilename = "halt";
//
//     _space->InitRegisters();
//     _space->RestoreState();
//
//     _thread->space = _space;
//     _thread->InitUserRegs();
//     _thread->Fork(_ForkThread, 1);
//
//     //currentThread->Yield();
//
//     delete executable;			// close file
//     delete _executable;
//
//     space->InitRegisters();		// set the initial register values
//     space->RestoreState();		// load page table register
//
//     printf("Starting first thread\n\n");
//     machine->Run();			// jump to the user progam
//     ASSERT(FALSE);			// machine->Run never returns;
// 					// the address space exits
// 					// by doing the syscall "exit"
// }

// 1 thread
void
StartProcess(char *filename)
{
    OpenFile *executable = fileSystem->Open(filename);
    AddrSpace *space;

    if (executable == NULL) {
	      printf("Unable to open file %s\n", filename);
	      return;
    }

    char* diskname = "disk";
    space = new AddrSpace(executable, diskname);
    currentThread->space = space;
    currentThread->userProgFilename = diskname;

    delete executable;			// close file

    space->InitRegisters();		// set the initial register values
    space->RestoreState();		// load page table register

    machine->Run();			// jump to the user progam
    ASSERT(FALSE);			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"

}

// Data structures needed for the console test.  Threads making
// I/O requests wait on a Semaphore to delay until the I/O completes.

static Console *console;
static Semaphore *readAvail;
static Semaphore *writeDone;
static SynchConsole * synchconsole;

//----------------------------------------------------------------------
// ConsoleInterruptHandlers
// 	Wake up the thread that requested the I/O.
//----------------------------------------------------------------------

static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

//----------------------------------------------------------------------
// ConsoleTest
// 	Test the console by echoing characters typed at the input onto
//	the output.  Stop when the user types a 'q'.
//----------------------------------------------------------------------

void
ConsoleTest (char *in, char *out)
{
    // char ch;
    // console = new Console(in, out, ReadAvail, WriteDone, 0);
    // readAvail = new Semaphore("read avail", 0);
    // writeDone = new Semaphore("write done", 0);
    //
    // for (;;) {
	  //    readAvail->P();		// wait for character to arrive
	  //    ch = console->GetChar();
	  //    console->PutChar(ch);	// echo it!
	  //    writeDone->P() ;        // wait for write to finish
	  //    if (ch == 'q') return;  // if q, quit
    // }
// for L5E6
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    char ch;
    synchconsole = new SynchConsole(in, out);
    for(;;) {
        ch = synchconsole->GetChar();
        synchconsole->PutChar(ch);
        if(ch == 'q') return;
    }
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
}
