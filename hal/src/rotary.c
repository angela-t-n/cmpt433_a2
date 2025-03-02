#include "hal/rotary.h"
#include <assert.h>

#include "hal/gpio.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>

// Referencing the btn_statemachine.c
// that was provided as an example code!
// Allow module to ensure it has been initialized (once!)
static bool is_initialized = false;

// based on the PWM document, the two GPIO required is GPIO 16 and 17.
// then, typing gpiofind GPIO16 and gpiofind GPIO17 respectively into console.
// BTW, they have the SAME chip number, so no point writing that down twice

// GPIO 16 for A line
#define GPIO_CHIP           GPIO_CHIP_2
#define GPIO_LINE_NUMBER_A  7

// GPIO 17 for B line
#define GPIO_LINE_NUMBER_B  8


// saves the gpio lines I assume?
struct GpioLine* s_lineA = NULL;
struct GpioLine* s_lineB = NULL;

// a counter. the one in button just always increases
// this one increases if it's clockwise, decreases if counter clockwise
static atomic_int counter = 0;



// Define the rotaryState machine?
// TODO: The Enums and whatnot.
struct rotaryStateEvent {
    // stores the rotaryState
    struct rotaryState* pNextrotaryState;

    // and stores the action associated with it, which is a func pointer
    void (*action)();
};

// 4 rotaryState events possible: rising and falling for A and B respectively
struct rotaryState {
    struct rotaryStateEvent A_rising;     // 0
    struct rotaryStateEvent A_falling;    // 1

    struct rotaryStateEvent B_rising;     // 2
    struct rotaryStateEvent B_falling;    // 3
};


// rotaryState machine stuff from the provided btn_rotaryStatemachine example file:

// flags to determine which way it started with
// decided to make it an int counter instead of a bool :(
// hoping that'd improve it's "accuracy"
static bool isCW = false;
static bool isCCW = false;

static char* direction = "not moved yet";

char* rotary_getDirection(){
    return direction;
}

void flag_cw(void)
{
    isCW = true;
    isCCW = false;
}

void flag_ccw(void)
{
    isCW = false;
    isCCW = true;
}

// bro it keeps sliding

// Instead of an on_release, I made 2 functions to do diff things
// clockwise or counter clockwise
static void on_clockwise(void){
    if(isCW){
        //printf("Rotated Clockwise\n");
        ++counter;
        direction = "CW";

        //printf("Counter: %d\n\n", counter);

        // reset the rotaryStates
        isCCW = isCW = false;
    }
}

static void on_counterclockwise(void){
    if(isCCW){
        //printf("Rotated Counter Clockwise\n");
        // lets not make it too negative
        if(counter > -10)
            --counter;
        direction = "CCW";

        //printf("Counter: %d\n\n", counter);

        // reset the rotaryStates
        isCCW = isCW = false;
    }
}

// A structure of rotaryStates that does diff things depending on what's happening
// Trying to follow the lecture and the images he drew
// but idk why it keeps sliding.
struct rotaryState rotaryStates[] = {
    { // rotaryState 0: Rest
        .A_rising =     {&rotaryStates[0], NULL},
        .A_falling =    {&rotaryStates[1], flag_cw}, // A fell first

        .B_rising =     {&rotaryStates[0], NULL},
        .B_falling =    {&rotaryStates[3], flag_ccw}, // B fell first
    },

    { // rotaryState 1:
        .A_rising =     {&rotaryStates[0], on_counterclockwise}, // back to rest (from counter clockwise)
        .A_falling =    {&rotaryStates[1], NULL},

        .B_rising =     {&rotaryStates[1], NULL},
        .B_falling =    {&rotaryStates[2], flag_cw}, // A fell then B fell
    },

    { // rotaryState 2: 
        .A_rising =     {&rotaryStates[3], flag_cw}, // A rise before B
        .A_falling =    {&rotaryStates[2], NULL},

        .B_rising =     {&rotaryStates[1], flag_ccw}, // B rise before A
        .B_falling =    {&rotaryStates[2], NULL},
    },


    { // rotaryState 3:
        .A_rising =     {&rotaryStates[3], NULL},
        .A_falling =    {&rotaryStates[2], flag_ccw}, // B fell then A fell

        .B_rising =     {&rotaryStates[0], on_clockwise}, // back to rest (from clockwise)
        .B_falling =    {&rotaryStates[3], NULL},
    },
};

// save the current rotaryState
struct rotaryState* pCurrentRotaryState = &rotaryStates[0];


// Initialize Line A and B
void rotary_init(void){
    // initialize the gpio to use for the rotary
    Gpio_initialize();

    // Initialize this once
    assert(!is_initialized);
    printf("Intializing Rotary Knob...\n");

    // open the lines to poll for events (if this is even the right term)
    s_lineA = Gpio_openForEvents(GPIO_CHIP, GPIO_LINE_NUMBER_A);
    s_lineB = Gpio_openForEvents(GPIO_CHIP, GPIO_LINE_NUMBER_B);

    is_initialized = true;
}

// Close Line A and B
void rotary_cleanup(void){
    // only cleanup if something has been initialized
    assert(is_initialized);
    printf("Stopping Rotary Knob...\n");

    // close the GPIO lines
    Gpio_close(s_lineA);
    Gpio_close(s_lineB);

    is_initialized = false;
}

// gets the current value of the counter
int rotary_getCounterVal(){
    return counter;
}


/* 
realized a lot of the code repeated.
Especially since it reads 1 line at a time...
I have no clue how to make it read like A AND B simutaneously
*/
void rotary_readLine(struct GpioLine* s_line, unsigned int GPIO_lineNum){
    assert(is_initialized);

    // get the bulk events for this line
    struct gpiod_line_bulk bulkEvents;
    int numEvents = Gpio_waitForLineChange(s_line, &bulkEvents);

    // iterate through all the events on this line
    for(int i = 0 ; i < numEvents ; i++){
        // get the line for this current event
        struct gpiod_line *lineHandle = gpiod_line_bulk_get_line(&bulkEvents, i);
        unsigned int currLineNumber = gpiod_line_offset(lineHandle);


        // get the event of this line
        struct gpiod_line_event currEvent;

        if(gpiod_line_event_read(lineHandle, &currEvent) == -1){
            printf("Error on line %d at iteration:%d\n", gpiod_line_offset(lineHandle),i);

            perror("failed to get event from line");
            exit(EXIT_FAILURE);
        }



        // check if it's rising
        bool isRising = currEvent.event_type == GPIOD_LINE_EVENT_RISING_EDGE;

        // manage 1 state at a time I assume?
        struct rotaryStateEvent* protaryStateEvent = NULL;

        // set the rotaryState events that's happening rn depending on which line
        bool isLine = currLineNumber == GPIO_lineNum;

        // check if it succeeded
        assert(isLine);
        
        // Check if it's rising or not
        if(isRising){
            switch(GPIO_lineNum){
                case GPIO_LINE_NUMBER_A:
                    // set the rotaryState event to rising for A
                    protaryStateEvent = &pCurrentRotaryState->A_rising;
                    break;

                case GPIO_LINE_NUMBER_B:
                    // set the rotaryState event to rising for B
                    protaryStateEvent = &pCurrentRotaryState->B_rising;
                    break;
            }
        }
        else{
            switch(GPIO_lineNum){
                case GPIO_LINE_NUMBER_A:
                    // set the rotaryState event to rising for A
                    protaryStateEvent = &pCurrentRotaryState->A_falling;
                    break;

                case GPIO_LINE_NUMBER_B:
                    // set the rotaryState event to rising for B
                    protaryStateEvent = &pCurrentRotaryState->B_falling;
                    break;
            }
        }


        // if there is an action to be done
        if(protaryStateEvent && protaryStateEvent->action){
            // perform the action stored as a func pointer
            protaryStateEvent->action();
        }

        // prints the current state index only if it changed while spinning
        #if 1
        pCurrentRotaryState = protaryStateEvent->pNextrotaryState;
        #endif

        #if 0
        int prevIndex = (int)(pCurrentRotaryState - rotaryStates);

        // update the current rotaryState
        pCurrentRotaryState = protaryStateEvent->pNextrotaryState;
        
        int currentStateIndex = (int)(pCurrentRotaryState - rotaryStates);

        if(prevIndex != currentStateIndex)
            printf("Current State: %d For line: %d\n", currentStateIndex, GPIO_lineNum);

        #endif
    }
}

void rotary_doState(){
    // Read line A first, then B
    rotary_readLine(s_lineA, GPIO_LINE_NUMBER_A);
    rotary_readLine(s_lineB, GPIO_LINE_NUMBER_B);
}
