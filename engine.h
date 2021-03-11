#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>

uint8_t strcopy(char *d, const char *s);

uint8_t getObjOrActorName(char *s, uint16_t id);
int8_t getObjectOrActorXY(uint16_t object, uint8_t *x, uint8_t *y);
uint16_t getObjActToObjActDist(uint16_t a, uint16_t b);

#endif