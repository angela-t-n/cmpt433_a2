#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#include "hal/gpio.h"
#include "hal/btn_statemachine.h"
#include "hal/lightSensor.h"
#include "hal/rotary.h"
#include "hal/lcdDisplay.h"

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


void testLCD(){
    lcdDisplay_init();

    //char buff[] = "meow meow nnyaa nyaa\nmeow meow nnyaa nyaa\nmeow meow nnyaa nyaa";
    // 21 characters wide
    // 12 characters long
    char buff[] = "123456789012345678901\n2\n3\n4\n5\n6\n7\n8\n9\n10\n1111111111111111111111111 222222222222222223 333333333333 ";
    lcdDisplay_updateScreen(buff);

    
    lcdDisplay_cleanup();
}



void testRotary(){
    rotary_init();

    while(true){
        // read it every 10 ms
        rotary_doState();
        milisecondSleep(10);
    }

    rotary_cleanup();
}

int main(void)
{
    

    /* irrelavant button press state machine example
    // Startup & Initialization
    Gpio_initialize();
    BtnStateMachine_init();

    // TESTING State machine
    while (true) {
        // TODO: This should be on it's own thread!
        BtnStateMachine_doState();

        printf("Counter at %+5d\n", BtnStateMachine_getValue());
    }

    BtnStateMachine_cleanup();
    Gpio_cleanup();

    printf("\nDone!\n");
    */

    // WORKS testing light sensor
    //testLightSensor();

    // TODO: pwmLED

    // WORKS
    //testLCD();

    // TODO: rotary :(
    //testRotary();

    return 0;
}
