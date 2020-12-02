#ifndef RESOURCE_H
#define RESOURCE_H

#include <stdint.h>

typedef uint8_t HROOM;

typedef struct Resource
{
    uint8_t room;
    uint16_t roomoffs;
} Resource;

#define _numCostumes 80
#define _numScripts 200
//#define _numSounds 100

#define READ_LE_UINT16(p) (*(uint16_t*)(p))

extern Resource costumes[];
extern Resource scripts[];

uint8_t readByte(HROOM f);
uint16_t readWord(HROOM f);
void readBuffer(HROOM r, uint8_t *buf, uint16_t sz);

HROOM openRoom(uint8_t i);
void closeRoom(HROOM r);
/* Opens a room and seeks to the beginning of the resource */
HROOM seekResource(Resource *res);
uint16_t readResource(HROOM r, uint8_t *buf, uint16_t sz);
void seekToOffset(HROOM r, uint16_t offs);
uint8_t readString(HROOM r, char *s);

#endif
