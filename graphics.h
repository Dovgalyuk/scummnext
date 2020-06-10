#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "resource.h"
#include "object.h"
#include "actor.h"

#define SCREEN_WIDTH 32

void initGraphics(void);

void decodeNESTrTable(void);
void decodeNESBaseTiles(void);
void decodeNESGfx(HROOM r);

void handleDrawing(void);

void graphics_loadCostumeSet(uint8_t n);
void graphics_print(const char *s);
void graphics_drawObject(Object *obj);
void graphics_drawActors(void);
void graphics_updateCostumes(void);

#endif
