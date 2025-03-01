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



struct GpioLine* s_lineA = NULL;
struct GpioLine* s_lineB = NULL;



// flags to know what's going on with the A line's "current" state
// should be updated every doState
static int prevStateA = 0;
static int prevStateB = 0;



// Define the State machine?
// TODO: The Enums and whatnot.





// Functions to use to run code depending on if its
// clockwise or counter clockwise
static void on_clockwise(void)
{
    printf("Rotating CLOCKWISE\n");
}

static void on_counterclockwise(void)
{
    printf("Rotating COUNTER-CLOCKWISE\n");
}



// Initialize Line A and B
void rotary_init(void){
    // initialize the gpio to use for the rotary
    Gpio_initialize();

    // Initialize this once
    assert(!is_initialized);

    // open the lines to poll for events (if this is even the right term)
    s_lineA = Gpio_openForEvents(GPIO_CHIP, GPIO_LINE_NUMBER_A);
    s_lineB = Gpio_openForEvents(GPIO_CHIP, GPIO_LINE_NUMBER_B);

    is_initialized = true;
}

// Close Line A and B
void rotary_cleanup(void){
    // only cleanup if something has been initialized
    assert(is_initialized);

    // close the GPIO lines
    Gpio_close(s_lineA);
    Gpio_close(s_lineB);

    is_initialized = false;
}




// read a line
int rotary_readLine(struct GpioLine* s_line, unsigned int GPIO_LINE_NUMBER){
    // initialize the structs we need.
    struct gpiod_line_bulk bulkEvents;
    struct gpiod_line_event event;

    // get the number of events for the line
    int numEvents = Gpio_waitForLineChange(s_line, &bulkEvents);

    // get the line's current state
    // 1 if rising
    // 0 if falling
    int currState = 0;

    for(int i = 0 ; i < numEvents ; i++){
        // get the line handle thingy
         struct gpiod_line *lineHandle = gpiod_line_bulk_get_line(&bulkEvents, i);

        // get the number for this line
        unsigned int currLineNumber = gpiod_line_offset(lineHandle);

        // get the line event
        if (gpiod_line_event_read(lineHandle, &event) == -1){
            perror("Line Event Err");
            exit(EXIT_FAILURE);
        }

        // check what each of the line's states are
        bool isLine = (currLineNumber == GPIO_LINE_NUMBER);

        // assert it to make sure it exists?
        assert(isLine);

        // check to see if it is rising: 1 (if it isn't then it's falling: 0)
        bool isRising = (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE);

        if(isLine){
            // update the values for this line's state
            if(isRising)
                currState = 1;
            else
                currState = 0;
        }
    }

    // return the result
    return currState;
}



// Given the 2 numbers, we can have a combination as follows:
    /*
    AB
    00	0
    01	1
    10	2
    11	3
    */

    /* However, we also need to determine another stage: whether or not it is in the middle of
    rotating a specific way, just starting to rotate that way, or finishing a rotation
    The reason why is as follows:

    A = __-----_____
    B = ____-----___
    
    This signals that it is rotating clockwise. It starts with A high (1) and B low (0)
    However, in the middle of it, A and B will both become 1 (or well, that is the hope)
    A will then become low again before B does, thus becoming 0 and 1 respectively.
    
    If we don't account for the fact that it is currently still "ending" the clockwise rotation
    we can mistakenly think that the ending A0 B1 pattern indicates it is 
    now turning counterclock wise, which is not true :(.

    As such, we need to somehow figure out a way to determine that it is starting to turn,
    in the middle of turning, and at the end of the turn.

    A way to determine rotation maybe could be observing the pattern of the bits

    Clockwise:
    A = __-----_____
    B = ____-----___
    00, then 10, then 11, then 01. Then the cycle continues with 00 (rest)

    Counter Clockwise:
    A = ____-----___
    B = __-----_____
    00, then 01, then 11, then 10. Then the cycle continues with 00 (rest)

    Maybe we can enum this? into words?
    */

typedef enum {
    ROTARY_NONE = 0,   // invalid jumps
    // Clockwise transitions:
    ROTARY_CW_START,   // 00 -> 10
    ROTARY_CW_MIDDLE,  // 10 -> 11
    ROTARY_CW_END,     // 11 -> 01
    ROTARY_CW_CYCLE,   // 01 -> 00  
    // Counterclockwise transitions:
    ROTARY_CCW_START,  // 00 -> 01
    ROTARY_CCW_MIDDLE, // 01 -> 11
    ROTARY_CCW_END,    // 11 -> 10
    ROTARY_CCW_CYCLE   // 10 -> 00
    
} RotaryTransition;

typedef enum{
    NONE,
    CLOCKWISE,
    COUNTERCLOCKWISE
} RotaryDirection;

// Then, combining that logic, we can kinda get the following combinations by combining
// prev A, prev B, curr A, curr B into a 4 bit code (16 total different combinations:)
// which can be hashed into an array of type rotary transition
// and looked up depending on index when you put the values together in that specific bit order
RotaryTransition transitionTable[16] = {
    ROTARY_NONE,        //00 00
    ROTARY_CCW_START,    //00 01
    ROTARY_CW_START,   //00 10
    ROTARY_NONE,        //00 11
    ROTARY_CW_CYCLE,    //01 00
    ROTARY_NONE,        //01 01
    ROTARY_NONE,        //01 10
    ROTARY_CCW_MIDDLE,  //01 11
    ROTARY_CCW_CYCLE,   //10 00
    ROTARY_NONE,        //10 01
    ROTARY_NONE,        //10 10
    ROTARY_CW_MIDDLE,   //10 11
    ROTARY_NONE,        //11 00
    ROTARY_CW_END,      //11 01
    ROTARY_CCW_END,     //11 10
    ROTARY_NONE         //11 11
};

static RotaryDirection prevDir = NONE;
void rotary_determineState(int currStateA, int currStateB){
    // Using the table and the types above, form the bits
    // using bitwise operators
    
    // firstly, bitshift prevA over to make room at the end
    // for example: 0x1 becomes 0x10
    int prevBits = prevStateA << 1;

    // then, or the prevB in to combine it
    // for example, 0x10 or 0x1 = 0x11
    prevBits = prevBits | prevStateB;

    // repeat with curr bits
    int currBits = currStateA << 1;
    currBits = currBits | currStateB;

    // then, combine both by shifting prevBits over by 2
    // for example, 0x11 << 2 becomes 0x1100
    int index = prevBits << 2;

    // and or-ing the curr bits to the end
    // for example, 0x1100 or 0x01 = 0x1101
    index = index | currBits;

    // now using this index, we can get the current direction it is going
    RotaryTransition currDir = transitionTable[index];

    printf("Prev: %d%d, Curr: %d%d, Index: %d, Transition: %d\n",
        prevStateA, prevStateB, currStateA, currStateB, index, currDir);
    
    // if we completed a full rotation, we can safely say we either went clockwise or counter clockwise!
    switch(currDir){
        case ROTARY_CW_START:
            printf("Start\t CLOCKWISE\n");
            fflush(stdout);
            prevDir = CLOCKWISE;
            break;
        case ROTARY_CW_MIDDLE:
            printf("mid\t CLOCKWISE\n");
            fflush(stdout);
            break;
        case ROTARY_CW_END:
            printf("End\t CLOCKWISE\n");
            fflush(stdout);
            break;
        case ROTARY_CW_CYCLE:
            on_clockwise();
            break;

        case ROTARY_CCW_START:
            printf("Start\t COUNTER CLOCKWISE\n");
            fflush(stdout);
            break;
        case ROTARY_CCW_MIDDLE:
            printf("mid\t COUNTER CLOCKWISE\n");
            fflush(stdout);
            break;
        case ROTARY_CCW_END:
            printf("End\t COUNTER CLOCKWISE\n");
            fflush(stdout);
            break;
        case ROTARY_CCW_CYCLE:
            on_counterclockwise();
            break;

        case ROTARY_NONE:
            prevDir = NONE;
            break;

        default:
            printf("Spinning :)\n");
    }
}



// Where the magic begins?
void rotary_doState(){
    static int stableStateA = 0;
    static int stableStateB = 0;
    
    int currStateA = rotary_readLine(s_lineA, GPIO_LINE_NUMBER_A);
    int currStateB = rotary_readLine(s_lineB, GPIO_LINE_NUMBER_B);

    // so far we're just printing it out.
    // TODO: Need help on converting these 1's and 0's into either clockwise or counter clockwise.
    //printf("\n\nState A: %d, \tState B: %d\n", currStateA, currStateB);

    if(currStateA == stableStateA && currStateB == stableStateB){
        // chuck it in the helper wohoo
        rotary_determineState(currStateA, currStateB);

        // update the state flags.
        prevStateA = currStateA;
        prevStateB = currStateB;
    }

    stableStateA = currStateA;
    stableStateB = currStateB;
}

void pain(){
    return;
    on_clockwise();
    on_counterclockwise();
}
