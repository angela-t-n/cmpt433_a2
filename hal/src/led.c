#include "hal/led.h"
#include <assert.h>

// This one uses PWM instead.

// Allow module to ensure it has been initialized (once!)
static bool is_initialized = false;

void ledInit(void){
    assert(!is_initialized);

}

void ledCleanup(void){
    assert(is_initialized);
    is_initialized = false;

}
