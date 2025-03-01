#ifndef _LCDDISPLAY_H_
#define _LCDDISPLAY_H_

void lcdDisplay_init();
void lcdDisplay_cleanup();

void lcdDisplay_updateScreen(char* message);

#endif