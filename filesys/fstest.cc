// fstest.cc
//	Simple test routines for the file system.
//
//	We implement:
//	   Copy -- copy a file from UNIX to Nachos
//	   Print -- cat the contents of a Nachos file
//	   Perftest -- a stress test for the Nachos file system
//		read and write a really large file in tiny chunks
//		(won't work on baseline system!)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "utility.h"
#include "filesys.h"
#include "system.h"
#include "thread.h"
#include "disk.h"
#include "stats.h"


#define TransferSize 	10 	// make it small, just to be difficult

//----------------------------------------------------------------------
// Copy
// 	Copy the contents of the UNIX file "from" to the Nachos file "to"
//----------------------------------------------------------------------

void
Copy(char *from, char *to)
{
    FILE *fp;
    OpenFile* openFile;
    int amountRead, fileLength;
    char *buffer;

// Open UNIX file
    if ((fp = fopen(from, "r")) == NULL) {
	printf("Copy: couldn't open input file %s\n", from);
	return;
    }

// Figure out length of UNIX file
    fseek(fp, 0, 2);
    fileLength = ftell(fp);
    fseek(fp, 0, 0);


// Create a Nachos file of the same length
    DEBUG('f', "Copying file %s, size %d, to file %s\n", from, fileLength, to);
    if (!fileSystem->Create(to, fileLength)) {	 // Create Nachos file
	       printf("Copy: couldn't create output file %s\n", to);
	       fclose(fp);
	       return;
    }

    openFile = fileSystem->Open(to);
    ASSERT(openFile != NULL);


// Copy the data in TransferSize chunks
    buffer = new char[TransferSize];
    while ((amountRead = fread(buffer, sizeof(char), TransferSize, fp)) > 0)
	openFile->Write(buffer, amountRead);
    delete [] buffer;

// Close the UNIX and the Nachos files
    delete openFile;
    fclose(fp);
}

//----------------------------------------------------------------------
// Print
// 	Print the contents of the Nachos file "name".
//----------------------------------------------------------------------

void
Print(char *name)
{
    OpenFile *openFile;
    int i, amountRead;
    char *buffer;

    if ((openFile = fileSystem->Open(name)) == NULL) {
	printf("Print: unable to open file %s\n", name);
	return;
    }

    buffer = new char[TransferSize];
    while ((amountRead = openFile->Read(buffer, TransferSize)) > 0)
	for (i = 0; i < amountRead; i++)
	    printf("%c", buffer[i]);
    delete [] buffer;

    delete openFile;		// close the Nachos file
    return;
}

//----------------------------------------------------------------------
// PerformanceTest
// 	Stress the Nachos file system by creating a large file, writing
//	it out a bit at a time, reading it back a bit at a time, and then
//	deleting the file.
//
//	Implemented as three separate routines:
//	  FileWrite -- write the file
//	  FileRead -- read the file
//	  PerformanceTest -- overall control, and print out performance #'s
//----------------------------------------------------------------------

#define FileName 	"TestFile"
#define Contents 	"1234567890"
#define ContentSize 	strlen(Contents)
#define FileSize 	((int)(ContentSize * 5))

static void
FileWrite()
{
    OpenFile *openFile;
    int i, numBytes;

    printf("\nSequential write of %d byte file, in %d byte chunks\n",
	FileSize, ContentSize); // comment this out for L5E7

    if (!fileSystem->Create(FileName, 0)) { // change from 0 to FileSize
        printf("\nPerf test: can't create %s\n", FileName); // comment this out for L5E7
        return; // comment this out for L5E7
    }
    openFile = fileSystem->Open(FileName);
    if (openFile == NULL) {
	      printf("\nPerf test: unable to open %s\n", FileName);
	      return;
    }

    for (i = 0; i < FileSize; i += ContentSize) {
        numBytes = openFile->Write(Contents, ContentSize); // write Contents to openFile
        printf("Wrting byte %d to byte %d\n", i * 10, i * 10 + ContentSize - 1); // comment this out for L5E7

	      if (numBytes < 10) {
	          printf("\nPerf test: unable to write %s\n", FileName);
	          delete openFile;
	          return;
	      }
    }
    delete openFile;	// close file
}

static void
FileRead()
{
    OpenFile *openFile;
    char *buffer = new char[ContentSize];
    int i, numBytes;

    printf("\nSequential read of %d byte file, in %d byte chunks\n",
	FileSize, ContentSize); // comment this out for L5E7

    if ((openFile = fileSystem->Open(FileName)) == NULL) {
	      printf("\nPerf test: unable to open file %s\n", FileName);
	      delete [] buffer;
	      return;
    }

    for (i = 0; i < FileSize; i += ContentSize) {
        numBytes = openFile->Read(buffer, ContentSize);
        printf("Reading byte %d to byte %d\n", i * 10, i * 10 + ContentSize - 1); // comment this out for L5E7
	      if ((numBytes < 10) || strncmp(buffer, Contents, ContentSize)) {
	         printf("\nPerf test: unable to read %s\n", FileName);
	         delete openFile;
	         delete [] buffer;
	         return;
	      }

    }
    delete [] buffer;
    delete openFile;	// close file
}

void
PerformanceTest()
{
    printf("\nStarting file system performance test:\n");
    stats->Print();

    FileWrite();
    FileRead();
    if (!fileSystem->Remove(FileName)) {
      printf("Perf test: unable to remove %s\n", FileName);
      return;
    }

    printf("\n");
    stats->Print();
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
// for L5E4
void CreateDir(char* name) {
    fileSystem->Create(name, -1);
}

// for L5E7
void read(int arg) {
    FileRead();
}

void write(int arg) {

    FileWrite();

    char* name = new char;
    sprintf(name, "reader%d", arg);

    Thread* readerThread = new Thread(name);
    readerThread->setPriority(0);
    readerThread->Fork(read, 0);

    fileSystem->Remove(FileName);

}

void ReaderWriterTest() {
    printf("\nStarting file system reader-writer test:\n\n");

    Thread* writerThread1 = new Thread("writer1");
    writerThread1->setPriority(0);
    Thread* writerThread2 = new Thread("writer2");
    writerThread2->setPriority(0);

    Thread* readerThread1 = new Thread("reader3");
    readerThread1->setPriority(0);

    writerThread1->Fork(write, 1);
    readerThread1->Fork(read, 0);
    writerThread2->Fork(write, 2);

}

// for L5C2
void ConsoleToPipe() {
    printf("\nThread 1 writes to pipe.\n");

    char input[SectorSize + 1];
    printf("Input:\n");
    scanf("%s", input);

    fileSystem->WritePipe(input, strlen(input));
}

void PipeToConsole() {
    printf("\nThread 2 reads from pipe.\n");

    char output[SectorSize];
    int len = fileSystem->ReadPipe(output);
    output[len] = '\0';

    printf("Output:\n%s\n\n", output);
}

void PipeTestChild(int arg) {
    printf("\nThread 1 communicates with Thread 0.\n");
    printf("Thread 1 reads data from the pipe.\n");

    char data[SectorSize + 1];
    int len = fileSystem->ReadFromPipe(data, 0, currentThread->getThreadID());
    printf("\noutput: %s\n", data);
}

void PipeTest() {
    printf("Thread 0 communicates with Thread 1.\n");
    printf("Thread 0 writes data to the pipe.\n");
    char input[SectorSize + 1];
    printf("Input: ");
    scanf("%s", input);

    Thread* thread = new Thread("child");
    int from = currentThread->getThreadID();
    int to = thread->getThreadID();
    fileSystem->WriteToPipe(input, strlen(input), from, to);
    thread->Fork(PipeTestChild, 0);
    currentThread->Yield();
}
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
