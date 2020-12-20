#include <stdlib.h>
#include <string.h>
#include <arch/zxn/esxdos.h>
#include "object.h"
#include "debug.h"
#include "room.h"
#include "graphics.h"

#define _numLocalObjects 200

static Object objects[_numLocalObjects];
extern uint8_t objectOwnerTable[_numGlobalObjects];
extern uint8_t objectStateTable[_numGlobalObjects];

static Object *findLocalObjectSlot(void)
{
    Object *obj = objects;
    Object *end = objects + _numLocalObjects;
    for ( ; obj != end ; ++obj)
        if (!obj->obj_nr)
        {
            memset(obj, 0, sizeof(Object));
            return obj;
        }
    DEBUG_PUTS("No more object slots available\n");
    DEBUG_HALT;
}

Object *object_get(uint16_t id)
{
    if (!id)
        return NULL;

    Object *obj = objects;
    Object *end = objects + _numLocalObjects;
    for ( ; obj != end ; ++obj)
        if (obj->obj_nr == id)
            return obj;

    return NULL;
}

void objects_clear(void)
{
    // clean the objects
    Object *obj = objects;
    Object *end = objects + _numLocalObjects;
    for ( ; obj != end ; ++obj)
        obj->obj_nr = 0;
}

void setupRoomObjects(HROOM r)
{
    esx_f_seek(r, 20, ESX_SEEK_SET);
    uint8_t numObj = readByte(r);

    uint16_t delta = numObj * 2;

	for (uint8_t i = 0 ; i < numObj ; i++)
    {
        Object *od = findLocalObjectSlot();
        seekToOffset(r, 28 + i * 2);
        od->OBIMoffset = readWord(r);
        esx_f_seek(r, delta - 2, ESX_SEEK_FWD);
		od->OBCDoffset = readWord(r);
        seekToOffset(r, od->OBCDoffset);

		//resetRoomObject(od, room);
        uint16_t size = readWord(r); // 2 - object info size
        readWord(r); // 4 - nothing (0)
    	od->obj_nr = readWord(r); // 6
        readByte(r); // 8 - nothing (0)
		od->x = readByte(r); // 9
        uint8_t y = readByte(r); // 10
		od->y = y & 0x7F;

		od->parentstate = (y & 0x80) ? 8 : 0;

		od->width = readByte(r); // 11

		od->parent = readByte(r); // 12

        od->walk_x = readByte(r); // 13
        // walk_y resolution is 2 pixels
        od->walk_y = (readByte(r) & 0x1f) * 4; // 14
        uint8_t b15 = readByte(r); // 15
        od->actordir = b15 & 7;
        od->height = (b15 & 0xf8) / 8;
        // TODO: fix name offset
        od->nameOffs = readByte(r); // 16 - name offset

        DEBUG_PRINTF("Load object %u x=%u y=%u walkx=%u walky=%u w=%u h=%u p=%u ps=%u actdir=%u nameOffs=%u\n",
            od->obj_nr, od->x, od->y, od->walk_x, od->walk_y, od->width, od->height, od->parent, od->parentstate, od->actordir, od->nameOffs);
        DEBUG_PRINTF("-- OBIMOffset 0x%x OBCDOffset 0x%x\n", od->OBIMoffset, od->OBCDoffset);
        // list of events follows
        uint8_t ev = readByte(r);
        while (ev)
        {
            uint8_t offs = readByte(r);
            DEBUG_PRINTF("-- script for event 0x%x offs 0x%x\n", ev, offs);
            ev = readByte(r);
        }

        seekToOffset(r, od->OBCDoffset + od->nameOffs);
        DEBUG_PUTS("-- name: ");
        ev = readByte(r);
        while (ev)
        {
            DEBUG_PUTC(ev);
            ev = readByte(r);
        }
        DEBUG_PUTC('\n');
    }
}

Object *object_find(uint16_t x, uint16_t y)
{
    // convert y to tiles, because objects have tile coordinates
    y /= 8 / V12_Y_MULTIPLIER;
	// const int mask = (_game.version <= 2) ? kObjectState_08 : 0xF;
    Object *obj = objects;
    Object *end = objects + _numLocalObjects;
	for ( ; obj != end ; ++obj) {
        if (obj->obj_nr == 0)
            continue;
		// if ((_objs[i].obj_nr < 1) || getClass(_objs[i].obj_nr, kObjectClassUntouchable))
		// 	continue;

        if (obj->x <= x && obj->x + obj->width > x
            && obj->y <= y && obj->y + obj->height > y)
            return obj;

		// b = i;
		// do {
		// 	a = _objs[b].parentstate;
		// 	b = _objs[b].parent;
		// 	if (b == 0) {
		// 		if (_objs[i].x_pos <= x && _objs[i].width + _objs[i].x_pos > x &&
		// 		    _objs[i].y_pos <= y && _objs[i].height + _objs[i].y_pos > y)
		// 			return _objs[i].obj_nr;
		// 		break;
		// 	}
		// } while ((_objs[b].state & mask) == a);
	}

    return NULL;
}

void readGlobalObjects(HROOM r)
{
    int i;
	for (i = 0 ; i != _numGlobalObjects ; i++)
    {
	 	uint8_t tmp = readByte(r);
        //DEBUG_PRINTF("global %u owner %u state %u\n", i, tmp & 15, tmp >> 4);
        objectOwnerTable[i] = tmp & 15;//OF_OWNER_MASK;
        objectStateTable[i] = tmp >> 4;//OF_STATE_SHL;
	}
}

uint8_t object_getOwner(uint16_t id)
{
    return objectOwnerTable[id];
}

void object_setOwner(uint16_t id, uint8_t owner)
{
    DEBUG_PRINTF("Get owner %u for %u\n", owner, id);
    objectOwnerTable[id] = owner;
}

int8_t object_whereIs(uint16_t id)
{
	// Note: in MM v0 bg objects are greater _numGlobalObjects
	if (id >= _numGlobalObjects)
		return WIO_NOT_FOUND;

	if (id < 1)
		return WIO_NOT_FOUND;

	if (objectOwnerTable[id] != OF_OWNER_ROOM)
	{
		// for (i = 0; i < _numInventory; i++)
		// 	if (inventory[i] == id)
		// 		return WIO_INVENTORY;
		return WIO_NOT_FOUND;
	}

    Object *obj = object_get(id);
    if (obj)
    {
        // if (_objs[i].fl_object_index)
        // 	return WIO_FLOBJECT;
        return WIO_ROOM;
    }

	return WIO_NOT_FOUND;
}

uint16_t object_getVerbEntrypoint(uint16_t obj, uint16_t entry)
{
	if (object_whereIs(obj) == WIO_NOT_FOUND)
		return 0;

    uint16_t ptr = object_get(obj)->OBCDoffset;
    ptr += 15;

    uint8_t res = 0;
    HROOM r = openRoom(currentRoom);
    seekToOffset(r, ptr);
    do
    {
        uint8_t e = readByte(r);
        if (e == 0)
        {
            res = 0;
            break;
        }
        res = readByte(r);
        if (e == entry || e == 0xff)
            break;
    }
    while (1);
    closeRoom(r);

    //DEBUG_PRINTF("Verb entry for %d is %d\n", entry, res);
    return res;
}

uint8_t object_getState(uint16_t id)
{
    // I knew LucasArts sold cracked copies of the original Maniac Mansion,
    // at least as part of Day of the Tentacle. Apparently they also sold
    // cracked versions of the enhanced version. At least in Germany.
    //
    // This will keep the security door open at all times. I can only
    // assume that 182 and 193 each correspond to one particular side of
    // it. Fortunately this does not prevent frustrated players from
    // blowing up the mansion, should they feel the urge to.

    // if (_game.id == GID_MANIAC && _game.version != 0 && (obj == 182 || obj == 193))
    // 	_objectStateTable[obj] |= kObjectState_08;

    return objectStateTable[id];
}

void object_setState(uint16_t id, uint8_t s)
{
    objectStateTable[id] = s;
}

void object_getXY(uint16_t id, uint8_t *x, uint8_t *y)
{
    Object *obj = object_get(id);
    *x = obj->walk_x;
    *y = obj->walk_y;

    // x = x >> V12_X_SHIFT;
    // y = y >> V12_Y_SHIFT;
}
