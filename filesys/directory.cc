// directory.cc
//	Routines to manage a directory of file names.
//
//	The directory is a table of fixed length entries; each
//	entry represents a single file, and contains the file name,
//	and the location of the file header on disk.  The fixed size
//	of each directory entry means that we have the restriction
//	of a fixed maximum size for file names.
//
//	The constructor initializes an empty directory of a certain size;
//	we use ReadFrom/WriteBack to fetch the contents of the directory
//	from disk, and to write back any modifications back to disk.
//
//	Also, this implementation has the restriction that the size
//	of the directory cannot expand.  In other words, once all the
//	entries in the directory are used, no more files can be created.
//	Fixing this is one of the parts to the assignment.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "filehdr.h"
#include "directory.h"
#include "filesys.h"


//----------------------------------------------------------------------
// Directory::Directory
// 	Initialize a directory; initially, the directory is completely
//	empty.  If the disk is being formatted, an empty directory
//	is all we need, but otherwise, we need to call FetchFrom in order
//	to initialize it from disk.
//
//	"size" is the number of entries in the directory
//----------------------------------------------------------------------

Directory::Directory(int size)
{
    table = new DirectoryEntry[size];
    tableSize = size;
    for (int i = 0; i < tableSize; i++)
	     table[i].inUse = FALSE;
}

//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------

Directory::~Directory()
{
    delete [] table;
}

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------

void
Directory::FetchFrom(OpenFile *file)
{
    (void) file->ReadAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------

void
Directory::WriteBack(OpenFile *file)
{
    (void) file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::FindIndex(char *name)
{
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse && !strncmp(table[i].name, name, FileNameMaxLen))

	          return i;
    return -1;		// name not in directory
}

//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't
//	in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::Find(char *name)
{
    int i = FindIndex(name);

    if (i != -1)
	     return table[i].sector;
    return -1;
}

//----------------------------------------------------------------------
// Directory::Add
// 	Add a file into the directory.  Return TRUE if successful;
//	return FALSE if the file name is already in the directory, or if
//	the directory is completely full, and has no more space for
//	additional file names.
//
//	"name" -- the name of the file being added
//	"newSector" -- the disk sector containing the added file's header
//----------------------------------------------------------------------

bool
Directory::Add(char *name, int newSector, int type)
{
/*
    if (FindIndex(name) != -1)
	     return FALSE;

    for (int i = 0; i < tableSize; i++)
        if (!table[i].inUse) {
            table[i].inUse = TRUE;
            table[i].sector = newSector;
            strncpy(table[i].name, name, FileNameMaxLen);
            return TRUE;
	  }
    return FALSE;	// no space.  Fix when we have extensible files.
*/
// for L5E4
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    char fileName[FileNameMaxLen + 1];
    char* temp = GetFileName(name);
    strncpy(fileName, temp, FileNameMaxLen);

    if (FindIndex(fileName) != -1)
	     return FALSE;
    for (int i = 0; i < tableSize; i++)
       if (!table[i].inUse) {
           table[i].inUse = TRUE;
           table[i].sector = newSector;
           strncpy(table[i].path, name, FilePathMaxLen);
           strncpy(table[i].name, fileName, FileNameMaxLen);
           table[i].type = type;
           return TRUE;
   	   }
    return FALSE;	// no space.  Fix when we have extensible files.
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
}

//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory.
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------

bool
Directory::Remove(char *name)
{
    int i = FindIndex(name);

    if (i == -1)
	     return FALSE; 		// name not in directory
    table[i].inUse = FALSE;
    return TRUE;
}

//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory.
//----------------------------------------------------------------------

void
Directory::List()
{
   for (int i = 0; i < tableSize; i++)
	     if (table[i].inUse) {
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
           //printf("%s, %d\n", table[i].name, table[i].sector);
           if(table[i].type == 1)
              MyPrint(i);
           else {
              MyPrint(i);
              OpenFile* directoryFile = new OpenFile(table[i].sector);
              Directory* directory = new Directory(NumDirEntries);
              directory->FetchFrom(directoryFile);
              directory->List();
              delete directory;
              delete directoryFile;
           }
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
       }
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------

void
Directory::Print()
{
    FileHeader *hdr = new FileHeader;
    char* temp;

    printf("\nDirectory contents:\n");
    for (int i = 0; i < tableSize; i++)
	     if (table[i].inUse) {
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
          if(table[i].type == 0)
              temp = "directory";
          else
              temp = "file";

          if(table[i].type == 1) {
              printf("\nName: %s, Sector: %d, Type: %s, Path: root/%s\n", table[i].name, table[i].sector, temp, table[i].path);
              hdr->FetchFrom(table[i].sector);
    	        hdr->Print();
          }
          else {
              printf("\nName: %s, Sector: %d, Type: %s, Path: root/%s\n", table[i].name, table[i].sector, temp, table[i].path);
              OpenFile* directoryFile = new OpenFile(table[i].sector);
              Directory* directory = new Directory(NumDirEntries);
              directory->FetchFrom(directoryFile);
              directory->Print();
              delete directory;
              delete directoryFile;
          }
	     }
    printf("\n");
    delete hdr;
}

// for L5E4
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void Directory::MyPrint(int i) {
    FileHeader *hdr = new FileHeader;
    hdr->FetchFrom(table[i].sector);

    char* temp;
    if(table[i].type == 0)
        temp = "directory";
    else
        temp = "file";

    printf("\nName: %s, Sector: %d, Type: %s, Path: root/%s\n", table[i].name, table[i].sector, temp, table[i].path);
    printf("Create time: %s\n", hdr->getCreateTime());
    printf("Last visited time: %s\n", hdr->getLastVisitedTime());
    printf("Last modified time: %s\n\n", hdr->getLastModifiedTime());

}

int Directory::GetLastDirSector(char* name) {
    int sector = DirectorySector;
    OpenFile* directoryFile = new OpenFile(sector);
    Directory* directory = new Directory(NumDirEntries);
    directory->FetchFrom(directoryFile); // root dir

    int strPos = 0, substrPos = 0;
    int len = strlen(name);
    char subStr[FileNameMaxLen + 1];

    while(strPos < len) {
        subStr[substrPos ++] = name[strPos ++];

        if(name[strPos] == '/') {
            subStr[substrPos] = '\0';
            sector = directory->Find(subStr);
            directoryFile = new OpenFile(sector);
            directory = new Directory(NumDirEntries);
            directory->FetchFrom(directoryFile); // next level dir
            strPos ++;
            substrPos = 0;
        }
    }
    return sector; // sector of last dir's filehead
}

int Directory::GetType(char* filename) {

    int index = FindIndex(filename);
    if(index != -1) // found
        return table[index].type;

    return -1;
}

char* Directory::GetFileName(char* name) {
    char fileName[FileNameMaxLen + 1];
    int pos = -1;
    int len = strlen(name);

    for(int i = len - 1; i >= 0; i --) {
        if(name[i] == '/') {
            pos = i + 1;
            break;
          }
    }
    if(pos == -1) pos = 0;

    int j = 0;
    for(int i = pos; i < len; i ++)
        fileName[j ++] = name[i];
    fileName[j] = '\0';
    return fileName;
}
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
