// for L5E6
#include "synchconsole.h"

static Semaphore *readAvail = new Semaphore("read avail", 0);
static Semaphore *writeDone = new Semaphore("write done", 0);
static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }


SynchConsole::SynchConsole(char *readFile, char *writeFile) {
    lock = new Lock("console");
    console = new Console(readFile, writeFile, ReadAvail, WriteDone, 0);
}

void SynchConsole::PutChar(char ch) {
    lock->Acquire();
    console->PutChar(ch);
    writeDone->P();
    lock->Release();
}

char SynchConsole::GetChar() {
    lock->Acquire();
    readAvail->P();
    char ch = console->GetChar();
    lock->Release();
    return ch;
}
