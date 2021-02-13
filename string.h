#ifndef STRING_H
#define STRING_H

#include <stdint.h>

extern int8_t talkDelay;

uint8_t *message_new(void);
void message_print(const uint8_t *m);
void messages_update(void);

#endif
