#include "hal/pwmLed.h"
#include <assert.h>

#include <stdio.h>
#include <stdlib.h>

// This one uses PWM instead.
#define PWM_GPIO_12_PATH    "/dev/hat/pwm/GPIO12/"
#define DUTY_CYCLE_FILE     "duty_cycle"
#define PERIOD_FILE         "period"
#define ENABLE_FILE         "enable"

#define MAX_PERIOD_CYCLE    469754879
#define MIN_PERIOD_CYCLE    0

#define MIN_HZ              0
#define MAX_HZ              1000000000


// Allow module to ensure it has been initialized (once!)
static bool is_initialized = false;

void pwmLed_init(void){
    assert(!is_initialized);
    is_initialized = true;

    printf("Intializing PWM LED...\n");

}

void pwmLed_cleanup(void){
    assert(is_initialized);
    printf("Stopping PWM LED...\n");

    is_initialized = false;
}


void writeValueToFile(const char* fileName, const int val){
    // open up the file at that file to write to
    FILE* file = fopen(fileName, "w");

    // check if it was not successful
    if(file == NULL){
        perror(fileName);

        // ok maybe we shouldn't crash the program tbh
        exit(EXIT_FAILURE);
    }
    // completely write over and put in the new val
    fprintf(file, "%d", val);
    // done now, so close the file!
    fclose(file);
}


// turns off the pwm led
void pwmLed_off(){
    // just write 0 to the enable file
    writeValueToFile(PWM_GPIO_12_PATH ENABLE_FILE, 0);
}

// turns on the pwm led
void pwmLed_on(){
    // just write 0 to the enable file
    writeValueToFile(PWM_GPIO_12_PATH ENABLE_FILE, 1);
}

// flash the pwm led at a specific hz
// max is 1 gigahz
void pwmLed_setFlash(int hz){
    // make sure it's between those two numbers
    if (hz < MIN_HZ && hz > MAX_HZ){
        printf("Out of hz bounds :( either too fast or negative! \n");
        return;
    }

    // start figuring out the period and duty_cycles
    // in order to flash it at that hz
    // all done in nanoseconds btw :(

    int periodNanoseconds = MAX_HZ / hz;
    int dutyCycleNanoseconds = periodNanoseconds / 2;

    // write it to the files
    writeValueToFile(PWM_GPIO_12_PATH PERIOD_FILE, periodNanoseconds);
    writeValueToFile(PWM_GPIO_12_PATH DUTY_CYCLE_FILE, dutyCycleNanoseconds);
}