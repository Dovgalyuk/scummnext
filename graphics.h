#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "resource.h"
#include "object.h"
#include "actor.h"
#include "verbs.h"

#define SCREEN_TOP 4
#define SCREEN_WIDTH 32
#define LINE_WIDTH 40
#define LINE_GAP ((LINE_WIDTH - SCREEN_WIDTH) / 2)

#define V12_X_MULTIPLIER 8
#define V12_Y_MULTIPLIER 2
#define V12_X_SHIFT 3
#define V12_Y_SHIFT 1

#define SPRITE_CURSOR 0

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

void graphics_loadSpritePattern(uint8_t nextSprite, uint8_t tile, uint8_t mask, uint8_t sprpal);
void graphics_loadCostumeSet(uint8_t n);
void graphics_print(const char *s);
void graphics_drawObject(Object *obj);
void graphics_drawVerb(VerbSlot *v);

#endif
