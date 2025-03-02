#include "hal/lcdDisplay.h"

#include "DEV_Config.h"
#include "LCD_1in54.h"
#include "GUI_Paint.h"
#include "GUI_BMP.h"
#include <stdio.h>		//printf()
#include <stdlib.h>		//exit()
#include <signal.h>     //signal()
#include <stdbool.h>
#include <assert.h>

// this is using the provided example code for the assignment

static UWORD *s_fb;
static bool isInitialized = false;

void lcdDisplay_init()
{
    assert(!isInitialized);
    // I wanted intialize outputs for everything.
    printf("Intializing LCD Display...\n");

    // Exception handling:ctrl + c
    // signal(SIGINT, Handler_1IN54_LCD);
    
    // Module Init
	if(DEV_ModuleInit() != 0){
        DEV_ModuleExit();
        exit(0);
    }
	
    // LCD Init
    DEV_Delay_ms(2000);

	LCD_1IN54_Init(HORIZONTAL);
	LCD_1IN54_Clear(WHITE);

	LCD_SetBacklight(1023);

    UDOUBLE Imagesize = LCD_1IN54_HEIGHT*LCD_1IN54_WIDTH*2;
    if((s_fb = (UWORD *)malloc(Imagesize)) == NULL) {
        perror("Failed to apply for black memory");
        exit(0);
    }
    isInitialized = true;
}


void lcdDisplay_cleanup()
{
    assert(isInitialized);
    printf("Stopping LCD Display...\n");

    // Module Exit
    free(s_fb);
    s_fb = NULL;
	DEV_ModuleExit();
    isInitialized = false;
}

// literally just paints it white LOL
void lcdDisplay_clearScreen(){
    Paint_NewImage(s_fb, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, 0, WHITE, 16);
    Paint_Clear(WHITE);

    // Send the RAM frame buffer to the LCD (actually display it)
    // Option 1) Full screen refresh (~1 update / second)
    LCD_1IN54_Display(s_fb);
}

void lcdDisplay_updateScreen(char* message)
{
    assert(isInitialized);

    const int x = 5;
    //const int starty = 70;
    const int starty = 5;

    int y = starty;

    // Initialize the RAM frame buffer to be blank (white)
    //Paint_NewImage(s_fb, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, 0, WHITE, 16);
    //Paint_Clear(WHITE);
    lcdDisplay_clearScreen();

    // Draw into the RAM frame buffer
    // WARNING: Don't print strings with `\n`; will crash!

    // This is the only part ive written myself.
    // we need to split the message when we see a new line, so that it doesn't crash
    // can use strtok and use \n as a tokenizer! thus splitting it
    // we can also space each line by 20 so that they don't overlap when we print it at the respective x and y
    int lineSpacing = 20;
    char *line = strtok(message, "\n");

    while (line != NULL) {
        // print the line
        Paint_DrawString_EN(x, y, line, &Font16, WHITE, BLACK);

        // move down by the spacing
        y += lineSpacing;

        // get the next line (already stored in line, so we're just getting the next segment
        // by moving the pointer)
        line = strtok(NULL, "\n"); 
    }

    // Send the RAM frame buffer to the LCD (actually display it)
    // Option 1) Full screen refresh (~1 update / second)
    LCD_1IN54_Display(s_fb);
    
    // Option 2) Update just a small window (~15 updates / second)
    //           Assume font height <= 20
    //LCD_1IN54_DisplayWindows(x, y, LCD_1IN54_WIDTH, y+20, s_fb);
	
}
