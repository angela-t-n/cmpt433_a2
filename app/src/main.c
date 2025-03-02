#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#include "hal/gpio.h"
#include "hal/btn_statemachine.h"
#include "hal/lightSensor.h"
#include "hal/rotary.h"
#include "hal/lcdDisplay.h"
#include "hal/pwmLed.h"

#include "sleepHelper.h"

#include <time.h>

void printDoubleArrAndFree(double* arr, const int* size){
    // print out the history for this past 1 ms
    for (int i = 0; i < *size; i++)
    {
        if(i%10 == 0){
            // make a new line, 10 entries per line.
            printf("\n");
        }

        printf("%f ", arr[i]);
    }
    // then demalloc it
    free(arr);
}

void printTestLightSensor(){
    printf("\n\nHistory:\n");
    // read the num and print it out]

    // get the history
    int historySize = 0;
    double *historyArr = lightSensor_getHistory(&historySize);
    printDoubleArrAndFree(historyArr, &historySize);
    
    printf("\n\n");

    // print out the weighted average thing
    double avg = lightSensor_getAverageReading();
    long long total = lightSensor_getNumSamplesTaken();
    printf("Current Average: %f, \t total Samples: %lld\n", avg, total);
}

// 1.1
void testLightSensor()
{
    lightSensor_init();

    int counter = 0;
    while (true)
    {
        lightSensor_moveCurrentDataToHistory();

        // every 100 ms, get the history
        if (counter == 99)
        {
            printTestLightSensor();

            // resets
            counter = 0;
        }

        // sleep for 1ms
        counter++;
        milisecondSleep(1);
    }

    lightSensor_cleanup();
}

// 1.2 is UDP

// 1.3 analyze for light dips

// 1.4 nice terminal output

// 1.5
// it is what it is
void testRotary(){
    rotary_init();

    while(true){
        // read it every 10 ms
        rotary_doState();
        milisecondSleep(20);
    }

    rotary_cleanup();
}

void testPwmLed(){
    pwmLed_init();

    pwmLed_setFlash(5);

    pwmLed_on();

    // stay on for like 3 seconds
    secondSleep(3);

    // turn it off
    pwmLed_off();

    // clean
    pwmLed_cleanup();
}



// 1.6
void testLCD(){
    lcdDisplay_init();

    //char buff[] = "meow meow nnyaa nyaa\nmeow meow nnyaa nyaa\nmeow meow nnyaa nyaa";
    // 21 characters wide
    // 12 characters long
    char buff[] = "123456789012345678901\n2\n3\n4\n5\n6\n7\n8\n9\n10\n1111111111111111111111111 222222222222222223 333333333333 ";
    lcdDisplay_updateScreen(buff);

    // after like 3 seconds, clear the screen
    secondSleep(3);

    lcdDisplay_clearScreen();

    lcdDisplay_cleanup();
}


// 1.7 use the periodTimer.h/.c provided on the website

// end of part 1. Submit as a tar


// 2. is not part of this program
// instead, i gdb debug the noworky.c file
// submit the debugging session text
// take a screenshot of debugging it in vscode


int main(void)
{
    

    return 0;
}
