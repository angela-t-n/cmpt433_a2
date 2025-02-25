// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <stdio.h>
#include <stdbool.h>
#include "hal/button.h"
#include "draw_stuff.h"

int main()
{
    printf("Hello world with LCD!\n");

    // Initialize all modules; HAL modules first
    button_init();
    DrawStuff_init();

  
    for (int i = 0; i < 100; i++) {
        char buff[1024];
        snprintf(buff, 1024, "Hello count: %3d", i);
        DrawStuff_updateScreen(buff);
    }
    

    // Cleanup all modules (HAL modules last)
    DrawStuff_cleanup();
    button_cleanup();

    printf("!!! DONE !!!\n"); 

    
}