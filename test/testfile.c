#include "syscall.h"

int fd1, fd2;
int res;
char buffer[20];

int main() {
    Create("write.txt");
    fd1 = Open("read.txt");
    fd2 = Open("write.txt");
    res = Read(buffer, 20, fd1);
    Write(buffer, res, fd2);
    Close(fd1);
    Close(fd2);
    Halt();
}
