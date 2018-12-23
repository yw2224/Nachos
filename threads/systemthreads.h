/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "copyright.h"
#include "thread.h"

class SystemThreads
{
private:
    int allocated;
    Thread* threadPtr;
public:
    int getAllocated() { return allocated; }
    void setAllocated(int arg) { allocated = arg; }
    Thread* getThreadPtr() { return threadPtr; }
    void setThreadPtr(Thread* arg) { threadPtr = arg; }
};

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/