#ifndef _PWMLED_H_
#define _PWMLED_H_

#include <stdbool.h>

void pwmLed_init(void);
void pwmLed_cleanup(void);

void pwmLed_on();
void pwmLed_off();

void pwmLed_setFlash(int);
int pwmLed_getHz(void);

#endif