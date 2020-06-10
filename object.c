#include <stdlib.h>
#include <string.h>
#include <arch/zxn/esxdos.h>
#include "object.h"
#include "debug.h"
#include "room.h"

#define _numLocalObjects 200

static Object objects[_numLocalObjects];

static Object *findLocalObjectSlot(void)
{
    uint8_t i = 1;
    for ( ; i < _numLocalObjects ; ++i)
        if (!objects[i].obj_nr)
        {
            memset(&objects[i], 0, sizeof(Object));
            return &objects[i];
        }
    DEBUG_PUTS("No more object slots available\n");
    DEBUG_HALT;
}

Object *object_get(uint8_t id)
{
    if (!id)
        return NULL;

    // first slot is not used
    Object *obj = objects;
    Object *end = objects + _numLocalObjects - 1;
    for ( ; obj != end ; --end)
        if (end->obj_nr == id)
            return end;

    return NULL;
}

void setupRoomObjects(HROOM r)
{
    esx_f_seek(r, 20, ESX_SEEK_SET);
    uint8_t numObj = readByte(r);

    uint16_t delta = numObj * 2;

	for (uint8_t i = 0 ; i < numObj ; i++)
    {
        Object *od = findLocalObjectSlot();
        esx_f_seek(r, 28 + i * 2, ESX_SEEK_SET);
        od->OBIMoffset = readWord(r);
        esx_f_seek(r, delta - 2, ESX_SEEK_FWD);
		od->OBCDoffset = readWord(r);
        esx_f_seek(r, od->OBCDoffset, ESX_SEEK_SET);

		//resetRoomObject(od, room);
        uint16_t size = readWord(r); // 2 - object info size
        readWord(r); // 4 - nothing
    	od->obj_nr = readWord(r); // 6
        readByte(r); // 8 - ???
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
        readByte(r); // 16 - ???

        DEBUG_PRINTF("Load object %u x=%u y=%u walkx=%u walky=%u w=%u h=%u p=%u ps=%u actdir=%u\n",
            od->obj_nr, od->x, od->y, od->walk_x, od->walk_y, od->width, od->height, od->parent, od->parentstate, od->actordir);
        // list of events follows
        uint8_t ev = readByte(r);
        while (ev)
        {
            uint8_t offs = readByte(r); // or word??
            DEBUG_PRINTF("-- script for event 0x%x offs 0x%x\n", ev, offs);
            ev = readByte(r);
        }
        // object script follows
    }
}
