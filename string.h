#ifndef STRING_H
#define STRING_H

#include <stdint.h>

extern int8_t talkDelay;

uint8_t *message_new(void);
void message_print(uint8_t *m, uint8_t c);
void messages_update(void);

#endif
