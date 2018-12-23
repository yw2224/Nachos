// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "filehdr.h"
#include "directory.h"
#include "malloc.h"


extern void execFunc(int arg), forkFunc(int arg);

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions
//	are in machine.h.
//----------------------------------------------------------------------

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
int FIFO() {

    int pos = -1;

    for(int i = 0; i < TLBSize; i ++) {
        if(machine->tlb[i].valid == FALSE) {
            pos = i;
            //printf("TLB[%d] is empty!\n", i);
            break;
        }
    }
    // if tlb full, replace the earliest one
    int min = machine->tlb[0].saveTime;
    if(pos == -1) {
        pos = 0;
        //printf("TLB full: \n");
        //printf("TLB[%d].saveTime = %d\n", 0, machine->tlb[0].saveTime);
        for(int i = 1; i < TLBSize; i ++) {
            if(machine->tlb[i].saveTime < min) {
                  min = machine->tlb[i].saveTime;
                  pos = i;
            }
            //printf("TLB[%d].saveTime = %d\n", i, machine->tlb[i].saveTime);
        }
    }
    //printf("Replacing TLB[%d]...\n", pos);
}

int LRU() {

    int pos = -1;

    for(int i = 0; i < TLBSize; i ++) {
        if(machine->tlb[i].valid == FALSE) {
            pos = i;
            //printf("TLB[%d] is empty!\n", i);
            break;
        }
    }

    // if tlb full, replace the less recent used one
    int min = machine->tlb[0].lastUseTime;
    if(pos == -1) {
        pos = 0;
        //printf("TLB full: \n");
        //printf("TLB[%d].lastUseTime = %d\n", 0, machine->tlb[0].lastUseTime);
        for(int i = 1; i < TLBSize; i ++) {
            if(machine->tlb[i].lastUseTime < min) {
                  min = machine->tlb[i].lastUseTime;
                  pos = i;
            }
            //printf("TLB[%d].lastUseTime = %d\n", i, machine->tlb[i].lastUseTime);
        }
    }
    //printf("Replacing TLB[%d]...\n", pos);
    return pos;
}
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

class threadInfo {
  public:
    AddrSpace *space;
    int PC;
};

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt)) {
        DEBUG('a', "Shutdown, initiated by user program.\n");
   	    interrupt->Halt();
    }

// L6
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    else if((which == SyscallException) && (type == SC_Create)) {
        int address = machine->ReadRegister(4);
        char* name = new char[FileNameMaxLen + 1];
        int pos = 0;
        int data;

        while(true) {
            machine->ReadMem(address + pos, 1, &data);
            if(data == 0) {
                name[pos] = '\0';
                break;
            }
            name[pos] = char(data);
            pos ++;
        }

        printf("Syscall: Create file: %s\n", name);
        fileSystem->Create(name, MaxFileSize);
        machine->PCAdvance();
    }
    else if((which == SyscallException) && (type == SC_Open)) {
        int address = machine->ReadRegister(4);
        char name[FileNameMaxLen + 1];
        int pos = 0;
        int data;

        while(true) {
            machine->ReadMem(address + pos, 1, &data);
            if(data == 0) {
                name[pos] = '\0';
                break;
            }
            name[pos ++] = char(data);
        }

        printf("Syscall: Open file: %s\n", name);
        OpenFile *openFile = fileSystem->Open(name);
        machine->WriteRegister(2, int(openFile));
        machine->PCAdvance();
    }
    else if((which == SyscallException) && (type == SC_Close)) {
        int fd = machine->ReadRegister(4);
        OpenFile *openFile = (OpenFile*)fd;
        printf("Syscall: Close file\n");
        delete openFile;
        machine->PCAdvance();
    }
    else if((which == SyscallException) && (type == SC_Read)) {

        int buffer = machine->ReadRegister(4);
        int size = machine->ReadRegister(5);
        int fd = machine->ReadRegister(6);
        OpenFile *openFile = (OpenFile*)fd;
        char into[size];
        int ret;

        if(fd == 0) { // for L7 std input
            for(int i = 0; i < size; i ++)
                into[i] = getchar();
            ret = size;
        }
        else { // L6
            // char *into: the buffer to contain the data to be read from disk
            ret = openFile->Read(into, size);
            printf("Syscall: Read file\n");
        }

        for(int i = 0; i < ret; i ++) {
            // int addr: the virtual addr to write to
            // int size: the number of bytes to be written
            // int value: the data to be written
            machine->WriteMem(buffer + i, 1, int(into[i]));
        }


        machine->WriteRegister(2, ret);
        machine->PCAdvance();
    }
    else if((which == SyscallException) && (type == SC_Write)) {

        int buffer = machine->ReadRegister(4);
        int size = machine->ReadRegister(5);
        int fd = machine->ReadRegister(6);

        char from[size];
        int data;
        int ret;

        for(int i = 0; i < size; i ++) { // get the data to be written
            // int addr: the virtual addr to read from
            // int size: the number of bytes to read
            // int *value: the place to write the result
            machine->ReadMem(buffer + i, 1, &data);
            from[i] = char(data);
        }

        if(fd == 1) { // for L7 std output
            for(int i = 0; i < size; i ++)
              putchar(from[i]);
        }
        else { // else write to file
            printf("Syscall: Write file\n");
            OpenFile *openFile = (OpenFile*)fd;
            // char *from: the buffer containing the data to be written to disk
            ret = openFile->Write(from, size);
        }

        machine->WriteRegister(2, ret);
        machine->PCAdvance();
    }

    else if((which == SyscallException) && (type == SC_Exec)) {
        int address = machine->ReadRegister(4);
        Thread* newThread = new Thread("new thread");

        printf("Syscall: Execute new thread\n");
        newThread->Fork(execFunc, address);

        machine->WriteRegister(2, newThread->getThreadID());
        machine->PCAdvance();
    }

    else if((which == SyscallException) && (type == SC_Fork)) {
        printf("Syscall: Fork new thread\n");
        int funcPC = machine->ReadRegister(4);

        Thread *newThread = new Thread("fork thread");
        newThread->space = currentThread->space;
        newThread->userProgFilename = currentThread->userProgFilename;
        newThread->Fork(forkFunc, funcPC);

        machine->PCAdvance();
    }

    else if((which == SyscallException) && (type == SC_Yield)) {
        printf("Syscall: Yield '%s'\n\n", currentThread->getName());
        machine->PCAdvance();
        currentThread->Yield();
    }

    else if((which == SyscallException) && (type == SC_Join)) {
        printf("Syscall: Join\n\n");
        int threadID = machine->ReadRegister(4);

        while(systemThreads[threadID].getAllocated())
            currentThread->Yield();
        machine->PCAdvance();
    }

// L7
    else if((which == SyscallException) && (type == SC_Pwd)) {
        system("pwd");
        machine->PCAdvance();
    }
    else if((which == SyscallException) && (type == SC_Ls)) {
        system("ls");
        machine->PCAdvance();
    }

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

// L4
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    else if (which == PageFaultException) {
        // tlb miss
        if(machine->tlb != NULL) {

            //printf("\nTLB MISS!\n");

            int badVAddr = machine->registers[BadVAddrReg];
            int vpn = (unsigned) badVAddr / PageSize;

            //int pos = FIFO();
            int pos = LRU();

            // save the new tlb
            machine->tlb[pos].saveTime = stats->totalTicks;
            machine->tlb[pos].lastUseTime = stats->totalTicks;
            machine->tlb[pos].valid = true;
            machine->tlb[pos].virtualPage = vpn;
            machine->tlb[pos].physicalPage = machine->pageTable[vpn].physicalPage;
            machine->tlb[pos].use = FALSE;
            machine->tlb[pos].dirty = FALSE;
            machine->tlb[pos].readOnly = FALSE;
        }
        // page table miss, need to load page from disk
        else {

            OpenFile *diskFile = fileSystem->Open(currentThread->userProgFilename);
            if(diskFile == NULL)
                ASSERT(FALSE);

            int badVAddr = machine->registers[BadVAddrReg];
            int vpn = (unsigned) badVAddr / PageSize;

            printf("========Page fault at VA = 0x%x========\n", machine->registers[BadVAddrReg]);

            // exercise 1-7
            if(machine->invertedPageTable == NULL) {
                // find a place in mainMemory to load to
                int pos = machine->memoryManagement->Find();
                if(pos == -1) { // replace the physical page pointed by pageTable[0];
                    pos = machine->pageTable[0].physicalPage;
                    if(machine->pageTable[0].dirty == TRUE) {
                         diskFile->WriteAt(&(machine->mainMemory[pos * PageSize]),
                         PageSize, machine->pageTable[0].virtualPage * PageSize);
                         machine->pageTable[0].valid = FALSE;
                    }
                }

                printf("   Loading virtualPage[%d] from disk\n", vpn);
                diskFile->ReadAt(&(machine->mainMemory[pos * PageSize]), PageSize, vpn * PageSize);
                machine->pageTable[vpn].valid = TRUE;
                machine->pageTable[vpn].physicalPage = pos;
                machine->pageTable[vpn].use = FALSE;
                machine->pageTable[vpn].dirty = FALSE;
                machine->pageTable[vpn].readOnly = FALSE;
                printf("   Saved at physicalPage[%d]\n\n", pos);
            }
            // challenge 2 using invertedPageTable
            else {

                int pos = machine->memoryManagement->Find();
                if(pos == -1) { // replace the physical page pointed by pageTable[0];
                    pos = machine->invertedPageTable[0].physicalPage;
                    if(machine->invertedPageTable[0].dirty == TRUE) {
                        diskFile->WriteAt(&(machine->mainMemory[pos * PageSize]),
                        PageSize, machine->invertedPageTable[0].virtualPage * PageSize);
                        machine->invertedPageTable[0].valid = FALSE;
                    }
                }
                printf("   Loading virtualPage[%d] from disk\n", vpn);
                diskFile->ReadAt(&(machine->mainMemory[pos * PageSize]), PageSize, vpn * PageSize);
                machine->invertedPageTable[vpn].valid = TRUE;
                machine->invertedPageTable[vpn].physicalPage = pos;
                machine->invertedPageTable[vpn].use = FALSE;
                machine->invertedPageTable[vpn].dirty = FALSE;
                machine->invertedPageTable[vpn].readOnly = FALSE;
                machine->invertedPageTable[vpn].threadID = currentThread->getThreadID();
                printf("   Saved at physicalPage[%d]\n", pos);
            }

            delete diskFile;

        }
    }
    // handling syscall Exit
    else if((which == SyscallException) && (type == SC_Exit)) {
        //printf("\n");
        // for L6
        printf("Syscall: Exit\n");
        int status = machine->ReadRegister(4);
        printf("Program exits with status %d\n", status);
        printf("\n");

        // for exercise 4-7
        if(machine->invertedPageTable == NULL) {

            for (int i = 0; i < machine->pageTableSize; i++) {
                if(machine->pageTable[i].valid == TRUE) {
                    int physicalPage = machine->pageTable[i].physicalPage;
                    if(machine->memoryManagement->Test(physicalPage)) {
                        machine->memoryManagement->Clear(physicalPage);
                        machine->pageTable[i].valid = FALSE;
                        printf("Clearing physicalPage[%d]...\n", physicalPage);
                    }
                }
            }
        }

        // for challenge 2
        else {

            for(int i = 0; i < NumPhysPages; i ++) {
                if(machine->invertedPageTable[i].valid) {
                    printf("PP[%d]: VP[%d], thread %d\n", machine->invertedPageTable[i].physicalPage, machine->invertedPageTable[i].virtualPage, machine->invertedPageTable[i].threadID);
                }
            }
            printf("\n");
            for(int i = 0; i < NumPhysPages; i ++) {
                if((machine->invertedPageTable[i].threadID == currentThread->getThreadID()) && (machine->invertedPageTable[i].valid == TRUE)) {
                    int physicalPage = machine->invertedPageTable[i].physicalPage;
                    if(machine->memoryManagement->Test(physicalPage)) {
                        machine->memoryManagement->Clear(physicalPage);
                        machine->invertedPageTable[i].valid = FALSE;
                        printf("Clearing physicalPage[%d] for thread %d...\n", physicalPage, currentThread->getThreadID());
                    }
                }
            }
        }

        printf("Program exits. CurrentThread '%s' finished.\n\n", currentThread->getName());

        machine->PCAdvance();
        currentThread->Finish();

    }
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    else {
	      printf("Unexpected user mode exception %d %d\n", which, type);
	      ASSERT(FALSE);
    }
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void execFunc(int address) {

    char name[MaxFileSize + 1];
    int pos = 0;
    int data;
    while(true) {
        machine->ReadMem(address + pos, 1, &data);
        if(data == 0) {
            name[pos] = '\0';
            break;
        }
        name[pos ++] = char(data);
    }

    printf("New thread's userprog name: %s\n\n", name);
    OpenFile *executable = fileSystem->Open(name);
    AddrSpace *space;
    space = new AddrSpace(executable, name);

    currentThread->space = space;
    currentThread->userProgFilename = name;

    delete executable;

    space->InitRegisters();
    space->RestoreState();
    machine->Run();
}

void forkFunc(int funcPC) {

    currentThread->space->InitRegisters();
    currentThread->space->RestoreState();
    machine->WriteRegister(PCReg, funcPC);
    machine->WriteRegister(NextPCReg, funcPC + 4);
    machine->Run();
}
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
