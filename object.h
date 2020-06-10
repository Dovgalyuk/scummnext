#ifndef OBJECT_H
#define OBJECT_H

#include <stdint.h>
#include "resource.h"

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

typedef struct Object
{
    uint8_t obj_nr;
    uint8_t x, y;
    uint8_t walk_x, walk_y;
    uint8_t width, height;
    uint8_t actordir;
    uint8_t state;
    uint8_t parent;
    uint8_t parentstate;
    uint16_t OBIMoffset;
    uint16_t OBCDoffset;
} Object;

Object *object_get(uint8_t id);

void setupRoomObjects(HROOM r);

#endif
