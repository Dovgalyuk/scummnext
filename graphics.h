#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "resource.h"
#include "object.h"
#include "actor.h"
#include "verbs.h"

#define SCREEN_TOP 2
#define SCREEN_WIDTH 32
#define SCREEN_HEIGHT 16
#define LINE_WIDTH 40
#define LINE_GAP ((LINE_WIDTH - SCREEN_WIDTH) / 2)
#define TEXT_GAP (LINE_GAP + 2)
#define SENTENCE_HEIGHT 2
#define VERB_HEIGHT 4
#define INV_GAP (LINE_GAP + 2)
#define INV_WIDTH (SCREEN_WIDTH - 4)
#define INV_TOP (SCREEN_TOP + SCREEN_HEIGHT + SENTENCE_HEIGHT + VERB_HEIGHT)
#define INV_COLS 2
#define INV_ROWS 2

#define TILEMAP_BASE 0x6000
#define TILE_BASE 0x4000
#define TILE_BYTES 2
#define LINE_BYTES (LINE_WIDTH * TILE_BYTES)

#define V12_X_MULTIPLIER 8
#define V12_Y_MULTIPLIER 2
#define V12_X_SHIFT 3
#define V12_Y_SHIFT 1

#define SPRITE_CURSOR 0

/** Virtual screen identifiers */
enum VirtScreenNumber {
	kMainVirtScreen = 0,	// The 'stage'
	kTextVirtScreen = 1,	// In V0-V3 games: the area where text is printed
	kVerbVirtScreen = 2,	// The verb area
	kUnkVirtScreen = 3		// ?? Not sure what this one is good for...
};

extern uint8_t costdesc[];
extern uint8_t costlens[];
extern uint8_t costoffs[];

extern uint8_t costdata_id;

extern const uint8_t v1MMNESLookup[];

void initGraphics(void);

void decodeNESTrTable(void);
void decodeNESGfx(HROOM r);
void decodeRoomBackground(HROOM r);

void graphics_updateScreen(void);

void graphics_loadCostumeSet(uint8_t n);
void graphics_print(const char *s, uint8_t c);
void graphics_printSentence(const char *s);
void graphics_drawObject(Object *obj);
void graphics_drawVerb(VerbSlot *v);
void graphics_drawInventory(uint8_t slot, const char *s);

uint8_t graphics_findVirtScreen(uint8_t y);

#endif
