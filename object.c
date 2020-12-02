#include <stdlib.h>
#include <string.h>
#include <arch/zxn/esxdos.h>
#include "object.h"
#include "debug.h"
#include "room.h"
#include "graphics.h"

#define _numLocalObjects 200

static Object objects[_numLocalObjects];

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

void setupRoomObjects(HROOM r)
{
    // clean the objects
    Object *obj = objects;
    Object *end = objects + _numLocalObjects;
    for ( ; obj != end ; ++obj)
        obj->obj_nr = 0;

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
        od->walk_y = readByte(r) & 0x1f; // 14
        uint8_t b15 = readByte(r); // 15
        od->actordir = b15 & 7;
        od->height = (b15 & 0xf8) / 8;
        od->nameOffs = readByte(r); // 16 - name offset

        DEBUG_PRINTF("Load object %u x=%u y=%u walkx=%u walky=%u w=%u h=%u p=%u ps=%u actdir=%u nameOffs=%u\n",
            od->obj_nr, od->x, od->y, od->walk_x, od->walk_y, od->width, od->height, od->parent, od->parentstate, od->actordir, od->nameOffs);
        DEBUG_PRINTF("-- OBIMOffset 0x%x OBCDOffset 0x%x\n", od->OBIMoffset, od->OBCDoffset);
        // list of events follows
        uint8_t ev = readByte(r);
        while (ev)
        {
            uint8_t offs = readByte(r); // or word??
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

        // object script follows
        //od->scriptOffset = esx_f_fgetpos(r);
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
