#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "resource.h"

void initGraphics(void);

void decodeNESTrTable(void);
void decodeNESBaseTiles(void);
void decodeNESGfx(HROOM r);

void handleDrawing(void);

void graphics_print(const char *s);

#endif
