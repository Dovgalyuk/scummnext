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
extern uint8_t boxesMatrix[];

uint8_t boxes_adjustXY(uint8_t *px, uint8_t *py);
int8_t boxes_getNext(int8_t from, int8_t to);
uint8_t box_getClosestPtOnBox(uint8_t b, uint8_t x, uint8_t y, uint8_t *outX, uint8_t *outY);
uint8_t box_checkXYInBounds(uint8_t b, uint8_t x, uint8_t y);

#endif
