#ifndef _BTN_STATEMACHINE_H_
#define _BTN_STATEMACHINE_H_

#include <stdbool.h>

void BtnStateMachine_init(void);
void BtnStateMachine_cleanup(void);

int BtnStateMachine_getValue(void);

// TODO: This should be on a background thread (internal?)
void BtnStateMachine_doState();

#endif