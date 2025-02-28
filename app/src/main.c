#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#include "hal/gpio.h"
#include "hal/btn_statemachine.h"
#include "hal/lightSensor.h"
#include "hal/rotary.h"
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
    lightSensor_Init();

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

    lightSensor_Cleanup();
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
    testLightSensor();

    /* //GIVING UP ON SPINNY
    // testing spinny
    Gpio_initialize();
    rotary_Init();

    while(true){
        rotary_doState();
        milisecondSleep(10);
    }

    rotary_Cleanup();
    Gpio_cleanup();
    */

    return 0;
}
