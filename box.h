#ifndef BOX_H
#define BOX_H

#include <stdint.h>

#define MAX_BOXES 16

typedef enum {
	kBoxXFlip		= 0x08,
	kBoxYFlip		= 0x10,
	kBoxIgnoreScale	= 0x20,
	kBoxPlayerOnly	= 0x20,
	kBoxLocked		= 0x40,
	kBoxInvisible	= 0x80
} BoxFlags;

typedef struct Box {
    uint8_t uy;
    uint8_t ly;
    uint8_t ulx;
    uint8_t urx;
    uint8_t llx;
    uint8_t lrx;
    uint8_t mask;
    uint8_t flags;
} Box;

extern uint8_t numBoxes;
extern Box boxes[];

uint8_t boxes_adjust_xy(uint8_t *px, uint8_t *py);

#endif
