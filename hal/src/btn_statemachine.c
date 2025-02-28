// Sample state machine for one GPIO pin.

#include "hal/btn_statemachine.h"
#include "hal/gpio.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>

// Pin config info: GPIO 24 (Rotary Encoder PUSH)
//   $ gpiofind GPIO24
//   >> gpiochip0 10
#define GPIO_CHIP          GPIO_CHIP_0
#define GPIO_LINE_NUMBER   10


static bool isInitialized = false;

struct GpioLine* s_lineBtn = NULL;
static atomic_int counter = 0;

/*
    Define the Statemachine Data Structures
*/
struct stateEvent {
    struct state* pNextState;
    void (*action)();
};
struct state {
    struct stateEvent rising;
    struct stateEvent falling;
};


/*
    START STATEMACHINE
*/
static void on_release(void)
{
    counter++;
}

struct state states[] = {
    { // Not pressed
        .rising = {&states[0], NULL},
        .falling = {&states[1], NULL},
    },

    { // Pressed
        .rising = {&states[0], on_release},
        .falling = {&states[1], NULL},
    },
};
/*
    END STATEMACHINE
*/

struct state* pCurrentState = &states[0];



void BtnStateMachine_init()
{
    assert(!isInitialized);
    s_lineBtn = Gpio_openForEvents(GPIO_CHIP, GPIO_LINE_NUMBER);
    isInitialized = true;
}
void BtnStateMachine_cleanup()
{
    assert(isInitialized);
    isInitialized = false;
    Gpio_close(s_lineBtn);
}

int BtnStateMachine_getValue()
{
    return counter;
}

// TODO: This should be on a background thread!
void BtnStateMachine_doState()
{
    assert(isInitialized);

    //printf("\n\nWaiting for an event...\n");
    // while (true) {
        struct gpiod_line_bulk bulkEvents;
        int numEvents = Gpio_waitForLineChange(s_lineBtn, &bulkEvents);

        // Iterate over the event
        for (int i = 0; i < numEvents; i++)
        {
            // Get the line handle for this event
            struct gpiod_line *line_handle = gpiod_line_bulk_get_line(&bulkEvents, i);

            // Get the number of this line
            unsigned int this_line_number = gpiod_line_offset(line_handle);

            // Get the line event
            struct gpiod_line_event event;
            if (gpiod_line_event_read(line_handle,&event) == -1) {
                perror("Line Event");
                exit(EXIT_FAILURE);
            }


            // Run the state machine
            bool isRising = event.event_type == GPIOD_LINE_EVENT_RISING_EDGE;

            // Can check with line it is, if you have more than one...
            bool isBtn = this_line_number == GPIO_LINE_NUMBER;
            assert (isBtn);

            struct stateEvent* pStateEvent = NULL;
            
            if (isRising) {
                pStateEvent = &pCurrentState->rising;
            } else {
                pStateEvent = &pCurrentState->falling;
            } 

            // Do the action
            if (pStateEvent->action != NULL) {
                pStateEvent->action();
            }
            pCurrentState = pStateEvent->pNextState;

            // DEBUG INFO ABOUT STATEMACHINE
            #if 0
            int newState = (pCurrentState - &states[0]);
            double time = event.ts.tv_sec + event.ts.tv_nsec / 1000000000.0;
            printf("State machine Debug: i=%d/%d  line num/dir = %d %8s -> new state %d     [%f]\n", 
                i, 
                numEvents,
                this_line_number, 
                isRising ? "RISING": "falling", 
                newState,
                time);
            #endif
        }
    // }

}
