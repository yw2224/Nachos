// synchdisk.cc
//	Routines to synchronously access the disk.  The physical disk
//	is an asynchronous device (disk requests return immediately, and
//	an interrupt happens later on).  This is a layer on top of
//	the disk providing a synchronous interface (requests wait until
//	the request completes).
//
//	Use a semaphore to synchronize the interrupt handlers with the
//	pending requests.  And, because the physical disk can only
//	handle one operation at a time, use a lock to enforce mutual
//	exclusion.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synchdisk.h"
#include "system.h"

//----------------------------------------------------------------------
// DiskRequestDone
// 	Disk interrupt handler.  Need this to be a C routine, because
//	C++ can't handle pointers to member functions.
//----------------------------------------------------------------------

static void
DiskRequestDone (int arg)
{
    SynchDisk* disk = (SynchDisk *)arg;

    disk->RequestDone();
}

//----------------------------------------------------------------------
// SynchDisk::SynchDisk
// 	Initialize the synchronous interface to the physical disk, in turn
//	initializing the physical disk.
//
//	"name" -- UNIX file name to be used as storage for the disk data
//	   (usually, "DISK")
//----------------------------------------------------------------------

SynchDisk::SynchDisk(char* name)
{
    semaphore = new Semaphore("synch disk", 0);
    lock = new Lock("synch disk lock");
    disk = new Disk(name, DiskRequestDone, (int) this);

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
// for L5E7
    mutexLock = new Lock("mutex lock");
    for(int i = 0; i < NumSectors; i ++)
        sectorSem[i] = new Semaphore("sector", 1);

// for L5C1b
    cache = new Cache[CacheSize];
    for(int i = 0; i < CacheSize; i ++)
        cache[i].valid = 0;

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
}

//----------------------------------------------------------------------
// SynchDisk::~SynchDisk
// 	De-allocate data structures needed for the synchronous disk
//	abstraction.
//----------------------------------------------------------------------

SynchDisk::~SynchDisk()
{
    delete disk;
    delete lock;
    delete semaphore;

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
// for L5E7
    delete mutexLock;

    for(int i = 0; i < NumSectors; i ++)
      delete sectorSem[i];
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
}

//----------------------------------------------------------------------
// SynchDisk::ReadSector
// 	Read the contents of a disk sector into a buffer.  Return only
//	after the data has been read.
//
//	"sectorNumber" -- the disk sector to read
//	"data" -- the buffer to hold the contents of the disk sector
//----------------------------------------------------------------------

void
SynchDisk::ReadSector(int sectorNumber, char* data)
{
// bfr L5C1
    lock->Acquire();			// only one disk I/O at a time
    disk->ReadRequest(sectorNumber, data);
    semaphore->P();			// wait for interrupt
    lock->Release();

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
// // for L5C1b
//
//     lock->Acquire();			// only one disk I/O at a time
//
//     int pos = -1;
//     for(int i = 0; i < CacheSize; i ++) {
//         if(cache[i].valid && (cache[i].sector == sectorNumber)) {
//             pos = i;
//             break;
//         }
//     }
//
//     if(pos == -1) {
//         //if(sectorNumber >= 7)
//             //printf("Cache miss! Sector: %d\n", sectorNumber);
//         disk->ReadRequest(sectorNumber, data);
//         semaphore->P();			// wait for interrupt
//
//
//         int swap = -1;
//         for(int i = 0; i < CacheSize; i ++) {
//             if(cache[i].valid == 0) {
//                 swap = i;
//                 break;
//             }
//         }
//         if(swap == -1) {
//             int min = cache[0].lastVisitedTime;
//             int minPos = 0;
//             for(int i = 1; i < CacheSize; i ++) {
//                 if(cache[i].lastVisitedTime < min) {
//                     min = cache[i].lastVisitedTime;
//                     minPos = i;
//                 }
//             }
//             swap = minPos;
//         }
//         cache[swap].valid = 1;
//         cache[swap].dirty = 0;
//         cache[swap].sector = sectorNumber;
//         cache[swap].lastVisitedTime = stats->totalTicks;
//         bcopy(data, cache[swap].data, SectorSize);
//     }
//     else {
//         // for L5C1b test.
//         // TestFile is stored at Sector 7-8. Sector 0-6 are for BitMap and Directory.
//         if(sectorNumber >= 7)
//             printf("Cache hit! Sector: %d\n", sectorNumber);
//
//         cache[pos].lastVisitedTime = stats->totalTicks;
//         bcopy(cache[pos].data, data, SectorSize);
//     }
//
//     lock->Release();
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
}

//----------------------------------------------------------------------
// SynchDisk::WriteSector
// 	Write the contents of a buffer into a disk sector.  Return only
//	after the data has been written.
//
//	"sectorNumber" -- the disk sector to be written
//	"data" -- the new contents of the disk sector
//----------------------------------------------------------------------

void
SynchDisk::WriteSector(int sectorNumber, char* data)
{
    lock->Acquire();			// only one disk I/O at a time
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
// for L5C1b

    for(int i = 0; i < CacheSize; i ++) {
        if(cache[i].sector == sectorNumber) {
            cache[i].valid = 0;
        }
    }
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    disk->WriteRequest(sectorNumber, data);
    semaphore->P();			// wait for interrupt
    lock->Release();
}

//----------------------------------------------------------------------
// SynchDisk::RequestDone
// 	Disk interrupt handler.  Wake up any thread waiting for the disk
//	request to finish.
//----------------------------------------------------------------------

void
SynchDisk::RequestDone()
{
    semaphore->V();
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
// for L5E7
void SynchDisk::PlusReader(int sector) {
    mutexLock->Acquire();
    numReaders[sector] ++;
    if(numReaders[sector] == 1)
      sectorSem[sector]->P();
    // for L5E7 test
    //printf("Thread '%s' reading file. %d readers in total.\n", currentThread->getName(), numReaders[sector]);
    mutexLock->Release();
}

void SynchDisk::MinusReader(int sector) {
    mutexLock->Acquire();
    numReaders[sector] --;
    if(numReaders[sector] == 0)
      sectorSem[sector]->V();
    // for L5E7 test
    //printf("Thread '%s' finishes reading. %d readers in total.\n", currentThread->getName(), numReaders[sector]);
    mutexLock->Release();
}

void SynchDisk::BeginWrite(int sector){
    sectorSem[sector]->P();
    // for L5E7 test
    //printf("Thread '%s' begins writing.\n", currentThread->getName());
}

void SynchDisk::EndWrite(int sector){
    // for L5E7 test
    //printf("Thread '%s' finishes writing.\n", currentThread->getName());
    sectorSem[sector]->V();
}
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
