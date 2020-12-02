#ifndef VERBS_H
#define VERBS_H

#include <stdint.h>

#define _numVerbs 32//100

#define VERB_NAME_SIZE 32

typedef struct VerbSlot {
    // Common::Rect curRect;
    // Common::Rect oldRect;
    uint8_t verbid;
    uint8_t x, y;
    uint8_t curmode;
    // uint8 color, hicolor, dimcolor, bkcolor, type;
    // uint8 charset_nr, curmode;
    // uint16 saveid;
    // uint8 key;
    // bool center;
    // uint8 prep;
    // uint16 imgindex;
    char name[VERB_NAME_SIZE];
} VerbSlot;

extern VerbSlot verbs[];

void verb_kill(uint8_t slot);
uint8_t verb_getSlot(uint8_t verb, uint8_t mode);

#endif
