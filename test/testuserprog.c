#include "syscall.h"

void func() {
    Create("test1.txt");
}

int main() {
// test1
/*
    Exec("../test/halt");
    Yield();
    Exit(0);
*/
// test2
/*
    int id = Exec("../test/halt");
    Join(id);
*/
// test3
    Create("test2.txt");
    Fork(func);
}
