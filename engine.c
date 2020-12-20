#include "engine.h"
#include "object.h"
#include "resource.h"
#include "actor.h"
#include "room.h"
#include "helper.h"
#include "debug.h"

static uint8_t strcopy(char *d, const char *s)
{
    char *d0 = d;
    while (*d++ = *s++)
        ;
    // returns length in characters
    return d - d0 - 1;
}

static uint8_t getDist(uint8_t x, uint8_t y, uint8_t x2, uint8_t y2)
{
	uint8_t a = ABS(y - y2);
	uint8_t b = ABS(x - x2);
	return MAX(a, b);
}

uint8_t getObjOrActorName(char *s, uint16_t id)
{
    //	if (objIsActor(obj))
    if (id < ACTOR_COUNT)
    {
        DEBUG_PRINTF("Name for actor %d\n", id);
        return strcopy(s, actors[id].name);
    }

	// for (i = 0; i < _numNewNames; i++) {
	// 	if (_newNames[i] == obj) {
	// 		debug(5, "Found new name for object %d at _newNames[%d]", obj, i);
	// 		return getResourceAddress(rtObjectName, i);
	// 	}
	// }

    Object *obj = object_get(id);
    if (!obj->OBCDoffset)
        return 0;
    DEBUG_PRINTF("Name for object %d\n", id);
    HROOM r = openRoom(currentRoom);
    seekToOffset(r, obj->OBCDoffset + obj->nameOffs);
    uint8_t len = readString(r, s);
    closeRoom(r);
    return len;
}

int8_t getObjectOrActorXY(uint16_t object, uint8_t *x, uint8_t *y)
{
	if (object < ACTOR_COUNT)
    {
        Actor *act = &actors[object];
		if (act->room == currentRoom)
        {
			*x = act->x;
			*y = act->y;
			return 0;
		}
        else
        {
			return -1;
        }
	}

	switch (object_whereIs(object))
    {
	case WIO_NOT_FOUND:
		return -1;
	case WIO_INVENTORY:
        // TODO

		// if (objIsActor(_objectOwnerTable[object])) {
		// 	act = derefActor(_objectOwnerTable[object], "getObjectOrActorXY(2)");
		// 	if (act && act->isInCurrentRoom()) {
		// 		x = act->getRealPos().x;
		// 		y = act->getRealPos().y;
		// 		return 0;
		// 	}
		// }
		return -1;
	default:
		break;
	}

    object_getXY(object, x, y);
	return 0;
}

uint16_t getObjActToObjActDist(uint16_t a, uint16_t b)
{
	uint8_t x, y, x2, y2;

    if (a < ACTOR_COUNT && b < ACTOR_COUNT)
    {
        uint8_t r1 = actors[a].room;
        uint8_t r2 = actors[b].room;
    	if (r1 == r2 && r1 && r1 != currentRoom)
            return 0;
    }

	if (getObjectOrActorXY(a, &x, &y) == -1)
		return 0xFF;

	if (getObjectOrActorXY(b, &x2, &y2) == -1)
		return 0xFF;

	// if (acta && !actb) {
	// 	AdjustBoxResult r = acta->adjustXYToBeInBox(x2, y2);
	// 	x2 = r.x;
	// 	y2 = r.y;
	// }

	// Now compute the distance between the two points
	return getDist(x, y, x2, y2);
}
