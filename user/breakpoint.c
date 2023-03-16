/* program to cause a breakpoint trap */

#include <inc/lib.h>

void test(int n)
{
    if (n == 2)
    {
        asm volatile("int $3");
    }

    return test(n - 1);
}
void
umain(int argc, char **argv) {
    test(5);
}


