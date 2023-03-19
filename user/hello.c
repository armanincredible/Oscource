/* hello, world */
#include <inc/lib.h>

void
umain(int argc, char **argv) {
    char array[10] = "";
    array[-5] = 5;
    //cprintf("%s\n", array);
    cprintf("i am environment %08x\n", thisenv->env_id);
}
