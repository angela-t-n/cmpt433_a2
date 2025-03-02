#ifndef _ROTARY_H_
#define _ROTARY_H_

#include <stdbool.h>

void rotary_init(void);
void rotary_cleanup(void);

int rotary_getCounterVal();

void rotary_doState();

#endif