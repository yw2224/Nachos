// filehdr.cc
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector,
//
//      Unlike in a real system, we do not keep track of file permissions,
//	ownership, last modification date, etc., in the file header.
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"
#include "time.h"

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{
    numBytes = fileSize;
    numSectors  = divRoundUp(fileSize, SectorSize);

    if (freeMap->NumClear() < numSectors)
	     return FALSE;		// not enough space

// for L5E3, allocate space using indirect index
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    if(numSectors < NumDirect) { // no need to use indirect index
       for (int i = 0; i < numSectors; i++)
          dataSectors[i] = freeMap->Find();
    }
    else {
        for(int i = 0; i < NumDirect - 1; i ++)
            dataSectors[i] = freeMap->Find();

        dataSectors[NumDirect - 1] = freeMap->Find();
        int indirectIndex[IndirectIndexSize];
        for(int j = 0; j < numSectors - NumDirect + 1; j ++)
            indirectIndex[j] = freeMap->Find();

        synchDisk->WriteSector(dataSectors[NumDirect - 1], (char *)indirectIndex);
    }
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
//
// // for L5C1a, allocate continuous space using indirect index
// /*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
//     int freeSpace = freeMap->FindCont(numSectors);
//     //printf("free space: %d\n", freeSpace);
//
//     if(freeSpace != -1) {
//         if(numSectors < NumDirect) { // no need to use indirect index
//            for (int i = 0; i < numSectors; i++)
//               dataSectors[i] = freeSpace + i;
//         }
//         else {
//            int i;
//            for(i = 0; i < NumDirect - 1; i ++)
//               dataSectors[i] = freeSpace + i;
//
//            dataSectors[NumDirect - 1] = freeSpace + (i ++);
//            int indirectIndex[IndirectIndexSize];
//            for(int j = 0; j < numSectors - NumDirect + 1; j ++, i ++) {
//               indirectIndex[j] = freeSpace + i;
//            }
//            synchDisk->WriteSector(dataSectors[NumDirect - 1], (char *)indirectIndex);
//        }
//     }
//     else {
//         if(numSectors < NumDirect) { // no need to use indirect index
//            for (int i = 0; i < numSectors; i++)
//               dataSectors[i] = freeMap->Find();
//         }
//         else {
//            for(int i = 0; i < NumDirect - 1; i ++)
//               dataSectors[i] = freeMap->Find();
//
//            dataSectors[NumDirect - 1] = freeMap->Find();
//            int indirectIndex[IndirectIndexSize];
//            for(int j = 0; j < numSectors - NumDirect + 1; j ++)
//               indirectIndex[j] = freeMap->Find();
//
//            synchDisk->WriteSector(dataSectors[NumDirect - 1], (char *)indirectIndex);
//        }
//     }
// /*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void
FileHeader::Deallocate(BitMap *freeMap)
{
// for L5E3
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    if(numSectors < NumDirect) {
        for (int i = 0; i < numSectors; i++) {
           ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
           freeMap->Clear((int) dataSectors[i]);
        }
    }
    else {
        char* indirectIndex = new char[SectorSize];
        synchDisk->ReadSector(dataSectors[NumDirect - 1], indirectIndex);
        for(int i = 0; i < numSectors - NumDirect + 1; i ++)
            freeMap->Clear((int) indirectIndex[i * 4]);

        for(int j = 0; j < NumDirect; j ++)
            freeMap->Clear((int) dataSectors[j]);
    }
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk.
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk.
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
// for L5E3
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    int firstIndex = SectorSize * (NumDirect - 1); // 9 * 128 = 1152
    if(offset < firstIndex)
        return(dataSectors[offset / SectorSize]);
    else {
        int sector = (offset - firstIndex) / SectorSize;
        char* indirectIndex = new char[SectorSize];
        synchDisk->ReadSector(dataSectors[NumDirect - 1], indirectIndex);
        return int(indirectIndex[sector * 4]);
    }
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
/*    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    for (i = 0; i < numSectors; i++)
	     printf("%d ", dataSectors[i]);

    printf("\nFile contents:\n");
    for (i = k = 0; i < numSectors; i++) {
	      synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
	          if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
		            printf("%c", data[j]);
            else
		            printf("\\%x", (unsigned char)data[j]);
	      }
        printf("\n");
    }
    delete [] data;
*/
// for L5E3
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents:  File size: %d.  File blocks:\n", numBytes);
    if(numSectors < NumDirect) {
       for (i = 0; i < numSectors; i++)
          printf("%d ", dataSectors[i]);
    }
    else {
        for(int i = 0; i < NumDirect - 1; i ++)
          printf("%d ", dataSectors[i]);
        printf("\nIndirect index: %d\n", dataSectors[NumDirect - 1]);

        char *indirectIndex = new char[SectorSize];
        synchDisk->ReadSector(dataSectors[NumDirect - 1], indirectIndex);
        j = 0;
        for(int i = 0; i < numSectors - NumDirect + 1; i ++) {
            printf("%d ", int(indirectIndex[j]));
            j = j + 4;
        }
    }

    printf("\nFile contents:\n");
    if(numSectors < NumDirect) {
        for (i = k = 0; i < numSectors; i++) {
            synchDisk->ReadSector(dataSectors[i], data);
            for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
                if ('\040' <= data[j] && data[j] <= '\176')  // isprint(data[j])
                    printf("%c", data[j]);
                else
                    printf("\\%x", (unsigned char)data[j]);
            }
        }
        printf("\n");
    }
    else {

        for (i = k = 0; i < NumDirect - 1; i++) {
            printf("Sector: %d\n", dataSectors[i]);
            synchDisk->ReadSector(dataSectors[i], data);
            for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
                if ('\040' <= data[j] && data[j] <= '\176')  // isprint(data[j])
                    printf("%c", data[j]);
                else
                    printf("\\%x", (unsigned char)data[j]);
            }
            printf("\n");
        }

        char* indirectIndex = new char[SectorSize];
        synchDisk->ReadSector(dataSectors[NumDirect - 1], indirectIndex);
        for(i = 0; i < numSectors - NumDirect + 1; i ++) {
            printf("Sector: %d\n", int(indirectIndex[i * 4]));
            synchDisk->ReadSector(int(indirectIndex[i * 4]), data);
            for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
                if ('\040' <= data[j] && data[j] <= '\176')  // isprint(data[j])
                    printf("%c", data[j]);
                else
                    printf("\\%x", (unsigned char)data[j]);
            }
            printf("\n");
        }
    }
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
// for L5E2
char* FileHeader::getCurrentTime() {
    time_t timep;
    time(&timep);
    char *temp = asctime(gmtime(&timep));
    return temp;
}

void FileHeader::setCreateTime() {
    char* temp = getCurrentTime();
    strncpy(createTime, temp, 25);
    createTime[24] = '\0';
}

void FileHeader::setLastVisitedTime() {
    char* temp = getCurrentTime();
    strncpy(lastVisitedTime, temp, 25);
    lastVisitedTime[24] = '\0';
}

void FileHeader::setLastModifiedTime() {
    char* temp = getCurrentTime();
    strncpy(lastModifiedTime, temp, 25);
    lastModifiedTime[24] = '\0';
}

// for L5E5
bool FileHeader::ExtendLen(BitMap* freeMap, int bytes) {
    numBytes = numBytes + bytes;
    int initNumSectors = numSectors;
    numSectors = divRoundUp(numBytes, SectorSize);
    if(initNumSectors == numSectors) // no need to allocate new sector
        return TRUE;
    if(freeMap->NumClear() < numSectors - initNumSectors) // not enough disk sectors
        return FALSE;

    printf("Extending %d sectors\n", numSectors - initNumSectors);

    if(numSectors < NumDirect) { // no need to use indirect index
        for (int i = initNumSectors; i < numSectors; i++)
            dataSectors[i] = freeMap->Find();
    }

    else {
        if(initNumSectors < NumDirect) { // need to create an indirect index
            for(int i = initNumSectors; i < NumDirect - 1; i ++)
                dataSectors[i] = freeMap->Find();

            dataSectors[NumDirect - 1] = freeMap->Find();
            int indirectIndex[IndirectIndexSize];
            for(int j = 0; j < numSectors - NumDirect + 1; j ++) {
                indirectIndex[j] = freeMap->Find();
            }

            synchDisk->WriteSector(dataSectors[NumDirect - 1], (char *)indirectIndex);
        }
        else { // already has indirect index
            int indirectIndex[IndirectIndexSize];
            synchDisk->ReadSector(dataSectors[NumDirect - 1], (char *)indirectIndex);
            for(int j = initNumSectors - NumDirect + 1; j < numSectors - NumDirect + 1; j ++)
                indirectIndex[j] = freeMap->Find();
            synchDisk->WriteSector(dataSectors[NumDirect - 1], (char *)indirectIndex);
        }
    }
    return TRUE;
}
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
