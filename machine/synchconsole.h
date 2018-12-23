// for L5E6
#ifndef SYNCHCONSOLE_H
#define SYNCHCONSOLE_H


#include "synch.h"
#include "console.h"


class SynchConsole {
  public:
    SynchConsole(char *readFile, char *writeFile);
    ~SynchConsole();
    void PutChar(char ch);
    char GetChar();

  private:
    Console *console;
    Lock *lock;
};

#endif // SYNCHCONSOLE_H
