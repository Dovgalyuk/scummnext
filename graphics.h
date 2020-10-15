#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "resource.h"
#include "object.h"
#include "actor.h"

#define SCREEN_WIDTH 32

#define V12_X_MULTIPLIER 8
#define V12_Y_MULTIPLIER 2

extern uint8_t costdesc[];
extern uint8_t costlens[];
extern uint8_t costoffs[];

extern uint8_t costdata_id;
extern uint8_t spriteTiles[];

extern const uint8_t v1MMNESLookup[];

void initGraphics(void);

void decodeNESTrTable(void);
void decodeNESBaseTiles(void);
void decodeNESGfx(HROOM r);

void graphics_updateScreen(void);

void graphics_loadCostumeSet(uint8_t n);
void graphics_print(const char *s);
void graphics_drawObject(Object *obj);

#endif
