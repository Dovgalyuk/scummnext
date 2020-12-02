#ifndef CURSOR_H
#define CURSOR_H

#include <stdint.h>

extern uint16_t cursorX;
extern uint16_t cursorY;

void cursor_setState(uint8_t s);
void cursor_animate(void);

#endif
