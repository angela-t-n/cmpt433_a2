#include "putTogether.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void runThingy(){
    putTogether_init();
    putTogether_runLoop();
    putTogether_cleanup();
}

int main(void)
{
    runThingy();

    return 0;
}
