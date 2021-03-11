#ifndef OBJECT_H
#define OBJECT_H

#include <stdint.h>
#include "resource.h"

#define _numGlobalObjects 775
#define _numLocalObjects 200
#define _numInventory 80

enum ObjectStateV2 {
	kObjectStatePickupable = 1,
	kObjectStateUntouchable = 2,
	kObjectStateLocked = 4,

	// FIXME: Not quite sure how to name state 8. It seems to mark some kind
	// of "activation state" for the given object. E.g. is a door open?
	// Is a drawer extended? In addition it is used to toggle the look
	// of objects that the user can "pick up" (i.e. it is set in
	// o2_pickupObject together with kObjectStateUntouchable). So in a sense,
	// it can also mean "invisible" in some situations.
	kObjectState_08 = 8
};

enum WhereIsObject {
	WIO_NOT_FOUND = -1,
	WIO_INVENTORY = 0,
	WIO_ROOM = 1,
	WIO_GLOBAL = 2,
	WIO_LOCAL = 3,
	WIO_FLOBJECT = 4
};

#define OF_OWNER_ROOM 0x0f

typedef struct Object
{
    uint16_t obj_nr;
    uint8_t x, y;
    uint8_t walk_x, walk_y;
    uint8_t width, height;
    uint8_t actordir;
    //uint8_t state;
    uint8_t parent;
    uint8_t parentstate;
    uint8_t preposition;
    uint8_t room;
    uint8_t whereis;
    uint8_t nameOffs;
    uint16_t OBIMoffset;
    uint16_t OBCDoffset;
} Object;

Object *object_get(uint16_t id);
Object *object_find(uint16_t x, uint16_t y);
int8_t object_whereIs(uint16_t id);
uint16_t object_getVerbEntrypoint(uint16_t obj, uint16_t entry);
uint8_t object_getName(char *s, uint16_t id);
uint8_t object_getOwner(uint16_t id);
void object_setOwner(uint16_t id, uint8_t owner);

uint8_t object_getState(uint16_t id);
void object_setState(uint16_t id, uint8_t s);

void object_getXY(uint16_t id, uint8_t *x, uint8_t *y);

void setupRoomObjects(HROOM r);
void objects_clear(void);
void objects_redraw(void);

void readGlobalObjects(HROOM r);


void inventory_redraw(void);
void inventory_addObject(Object *obj, uint8_t room);
uint16_t inventory_checkXY(int8_t x, int8_t y);

#endif
