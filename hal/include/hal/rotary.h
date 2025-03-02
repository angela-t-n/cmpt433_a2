#ifndef _ROTARY_H_
#define _ROTARY_H_

#include <stdbool.h>

void rotary_init(void);
void rotary_cleanup(void);

int rotary_getCounterVal(void);
char* rotary_getDirection(void);

void rotary_doState(void);

#endif