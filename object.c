#include <stdlib.h>
#include <string.h>
#include <arch/zxn.h>
#include <arch/zxn/esxdos.h>
#include "object.h"
#include "debug.h"
#include "room.h"
#include "graphics.h"
#include "script.h"
#include "helper.h"

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
    //uint8_t room;
    //uint8_t whereis;
    uint8_t nameOffs;
    uint16_t OBIMoffset;
    uint16_t OBCDoffset;
} Object;


#define MAX_INV (INV_COLS * INV_ROWS)

extern Object objects[_numLocalObjects];
extern uint8_t objectOwnerTable[_numGlobalObjects];
extern uint8_t objectStateTable[_numGlobalObjects];
extern uint8_t inventoryOffset;
extern Inventory invObjects[_numInventory];

#define OBJECTS_PAGE 3
#define ENTER PUSH_PAGE(2, OBJECTS_PAGE)
#define EXIT POP_PAGE(2)

// static Object *findLocalObjectSlot(void)
// {
//     Object *obj = objects;
//     Object *end = objects + _numLocalObjects;
//     for ( ; obj != end ; ++obj)
//         if (!obj->obj_nr)
//         {
//             memset(obj, 0, sizeof(Object));
//             return obj;
//         }
//     DEBUG_PUTS("No more object slots available\n");
//     DEBUG_HALT;
// }

static Object *object_get(uint16_t id)
{
    if (!id)
        return NULL;

    //ENTER;
    Object *obj = objects;
    Object *end = objects + _numLocalObjects;
    for ( ; obj != end ; ++obj)
    {
        if (obj->obj_nr == id)
        {
            //EXIT;
            return obj;
        }
    }

    //EXIT;
    return NULL;
}

void objects_clear(void)
{
    ENTER;
    // clean the objects
    Object *obj = objects;
    Object *end = objects + _numLocalObjects;
    for ( ; obj != end ; ++obj)
        //if (obj->room == currentRoom)
            obj->obj_nr = 0;
    EXIT;
}

void setupRoomObjects(HROOM r)
{
    ENTER;
    esx_f_seek(r, 20, ESX_SEEK_SET);
    uint8_t numObj = readByte(r);

    uint16_t delta = numObj * 2;
//#define MAX_ROOM_OBJ 32
    //uint8_t offs[MAX_ROOM_OBJ];

    uint8_t i;
	for (i = 1 ; i <= numObj ; i++)
    {
        Object *od = &objects[i];//findLocalObjectSlot();
        //offs[i] = od - objects;
        seekToOffset(r, 28 + (i - 1) * 2);
        od->OBIMoffset = readWord(r);
        seekFwd(r, delta - 2);
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
        uint8_t b14 = readByte(r); // 14
        // walk_y resolution is 2 pixels
        od->walk_y = (b14 & 0x1f) * 4;
        od->preposition = b14 >> 5;
        uint8_t b15 = readByte(r); // 15
        od->actordir = b15 & 7;
        od->height = (b15 & 0xf8) / 8;
        // TODO: fix name offset
        od->nameOffs = readByte(r); // 16 - name offset

        // od->whereis = WIO_ROOM;
        // od->room = currentRoom;

        DEBUG_PRINTF("Load object %u x=%u y=%u walkx=%u walky=%u w=%u h=%u p=%u ps=%u actdir=%u prep=%u nameOffs=%u\n",
            od->obj_nr, od->x, od->y, od->walk_x, od->walk_y, od->width, od->height, od->parent,
            od->parentstate, od->actordir, od->preposition, od->nameOffs);
        DEBUG_PRINTF("-- OBIMOffset 0x%x OBCDOffset 0x%x\n", od->OBIMoffset, od->OBCDoffset);
        // list of events follows
        // uint8_t ev = readByte(r);
        // while (ev)
        // {
        //     uint8_t offs = readByte(r);
        //     // DEBUG_PRINTF("-- script for event 0x%x offs 0x%x\n", ev, offs);
        //     ev = readByte(r);
        // }

        seekToOffset(r, od->OBCDoffset + od->nameOffs);
        DEBUG_PUTS("-- name: ");
        uint8_t ev = readByte(r);
        while (ev)
        {
            DEBUG_PUTC(ev);
            ev = readByte(r);
        }
        DEBUG_PUTC('\n');
    }

	// for (i = 0 ; i < numObj ; i++)
    // {
    //     uint8_t k = offs[i];
    //     if (objects[k].parent)
    //     {
    //         objects[k].parent = offs[objects[k].parent - 1];
    //     }
    // }

    EXIT;
}

uint16_t object_find(uint16_t x, uint16_t y)
{
    ENTER;
    // convert y to tiles, because objects have tile coordinates
    y = y / (8 / V12_Y_MULTIPLIER);
    Object *obj = objects;
    Object *end = objects + _numLocalObjects;
	for ( ; obj != end ; ++obj) {
        if (obj->obj_nr == 0/* || obj->whereis != WIO_ROOM*/)
            continue;

        if (objectStateTable[obj->obj_nr] & kObjectStateUntouchable)
            continue;

        // if (obj->x <= x && obj->x + obj->width > x
        //     && obj->y <= y && obj->y + obj->height > y)
        // {
        //     DEBUG_PRINTF("Can find object %d parent %d\n", obj->obj_nr, obj->parent);
        // }
        //     return obj;

        Object *b = obj;
        uint8_t a;
        do
        {
            a = b->parentstate;
            // if (obj->x <= x && obj->x + obj->width > x
            //     && obj->y <= y && obj->y + obj->height > y)
            //     DEBUG_PRINTF("Testing obj %d parent %d\n", b->obj_nr, b->parent);
            if (b->parent == 0)
            {
                if (obj->x <= x && obj->x + obj->width > x
                    && obj->y <= y && obj->y + obj->height > y)
                {
                    uint16_t res = obj->obj_nr;
                    EXIT;
                    return res;
                }
                break;
            }
            b = &objects[b->parent];
        }
        while ((objectStateTable[b->obj_nr] & kObjectState_08) == a);
	}

    EXIT;
    return 0;
}

void readGlobalObjects(HROOM r)
{
    ENTER;
    int i;
	for (i = 0 ; i != _numGlobalObjects ; i++)
    {
	 	uint8_t tmp = readByte(r);
        //DEBUG_PRINTF("global %u owner %u state %u\n", i, tmp & 15, tmp >> 4);
        objectOwnerTable[i] = tmp & 15;//OF_OWNER_MASK;
        objectStateTable[i] = tmp >> 4;//OF_STATE_SHL;
	}
    EXIT;
}

uint8_t object_getOwner(uint16_t id)
{
    ENTER;
    uint8_t res = objectOwnerTable[id];
    EXIT;
    return res;
}

void object_setOwner(uint16_t id, uint8_t owner)
{
    ENTER;
    // DEBUG_PRINTF("Set owner %u for %u\n", owner, id);
    objectOwnerTable[id] = owner;
    EXIT;
}

int8_t object_whereIs(uint16_t id)
{
    DEBUG_PRINTF("Where is %d\n", id);
	// Note: in MM v0 bg objects are greater _numGlobalObjects
	if (!id || id >= _numGlobalObjects)
		return WIO_NOT_FOUND;

    // check inventory
    ENTER;
    uint8_t i;
    for (i = 0 ; i < _numInventory ; i++)
    {
        if (invObjects[i].obj_nr == id)
        {
            EXIT;
            return WIO_INVENTORY;
        }
    }

    Object *obj = object_get(id);
    EXIT;
    if (obj)
    {
        // if (objectOwnerTable[id] != OF_OWNER_ROOM)
        // {
        //     // for (i = 0; i < _numInventory; i++)
        //     // 	if (inventory[i] == id)
        //     // 		return WIO_INVENTORY;
        //     return WIO_NOT_FOUND;
        // }

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

    ENTER;
    uint16_t ptr = object_get(obj)->OBCDoffset;
    EXIT;
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

    ENTER;
    uint8_t res = objectStateTable[id];
    EXIT;
    return res;
}

void object_setState(uint16_t id, uint8_t s)
{
    ENTER;
    //DEBUG_PRINTF("Set state %d %x\n", id, s); 
    objectStateTable[id] = s;
    EXIT;
}

void object_getXY(uint16_t id, uint8_t *x, uint8_t *y)
{
    ENTER;
    Object *obj = object_get(id);
    *x = obj->walk_x;
    *y = obj->walk_y;

    // x = x >> V12_X_SHIFT;
    // y = y >> V12_Y_SHIFT;
    EXIT;
}

uint16_t object_getOBCDoffset(uint16_t id)
{
    ENTER;
    Object *obj = object_get(id);
    uint16_t res = obj->OBCDoffset;
    EXIT;
    return res;
}

uint16_t object_getId(Object *obj)
{
    ENTER;
    uint16_t res = obj->obj_nr;
    EXIT;
    return res;
}

void object_draw(uint16_t id)
{
    ENTER;
    Object *obj = object_get(id);
    //DEBUG_PRINTF("Draw object %d %x\n", id, obj);
    graphics_drawObject(obj->OBIMoffset, obj->x, obj->y, obj->width, obj->height);
    EXIT;
}

void objects_redraw(void)
{
    //DEBUG_PUTS("Redraw all\n");
    // clear screen
    HROOM r = openRoom(currentRoom);
    decodeRoomBackground(r);
    closeRoom(r);

    // Object *obj = objects;
    // Object *end = objects + _numObjects;
    // for ( ; obj != end ; ++obj)
    // {
    ENTER;
    int i;
    for (i = _numLocalObjects - 1 ; i >= 0 ; --i)
    {
        Object *obj = &objects[i];
        if (obj->obj_nr
            && (objectStateTable[obj->obj_nr] & kObjectState_08)
            /*&& obj->whereis == WIO_ROOM*/
            //&& obj->room == currentRoom
            // TODO: check room
            )
        {
            Object *od = obj;
            uint8_t a;
            do {
                a = od->parentstate;
                if (!od->parent)
                {
                    object_draw(obj->obj_nr);
                    //DEBUG_PRINTF("--- obj %d\n", obj->obj_nr);
                    break;
                }
                od = &objects[od->parent];
            } while ((objectStateTable[od->obj_nr] & kObjectState_08) == a);
        }
    }
    EXIT;
}

static uint8_t readName(char *s, uint8_t room, uint16_t obcd, uint8_t offs)
{
    if (!obcd)
        return 0;
    //DEBUG_PRINTF("Name for object %d\n", id);
    HROOM r = openRoom(room);
    seekToOffset(r, obcd + offs);
    uint8_t len = readString(r, s);
    closeRoom(r);
    return len;
}

uint8_t object_getName(char *s, uint16_t id)
{
    ENTER;
    Object *obj = object_get(id);
    uint8_t res = readName(s, currentRoom, obj->OBCDoffset, obj->nameOffs);
    EXIT;
    return res;
}

uint8_t object_getPreposition(uint16_t id)
{
    ENTER;
    Object *obj = object_get(id);
    uint8_t res = obj->preposition;
    EXIT;
    return res;
}

static uint8_t inventory_getCount(uint8_t owner)
{
    ENTER;
	uint8_t i;
    uint16_t obj;
	uint8_t count = 0;
	for (i = 0; i < _numInventory; i++) {
		obj = invObjects[i].obj_nr;
		if (obj && objectOwnerTable[obj] == owner)
			count++;
	}
    EXIT;
	return count;
}

static uint8_t inventory_getSlot(void)
{
	uint8_t i;
	uint8_t count = 0;
	for (i = 0; i < _numInventory; i++)
		if (!invObjects[i].obj_nr)
            return i;
    /* Should never happen */
    return 0;
}

// uint16_t inventory_find(uint8_t owner, uint8_t idx)
// {
// 	// uint8_t count = 1, i;
//     // uint16_t obj;
// 	// for (i = 0; i < _numInventory; i++) {
// 	// 	obj = inventory[i];
// 	// 	if (obj && object_getOwner(obj) == owner && count++ == idx)
// 	// 		return obj;
// 	// }
// 	return 0;
// }

void inventory_redraw(void)
{
    ENTER;
	// int inventoryArea = 48;
	// int maxChars = 13;

	//_mouseOverBoxV2 = -1;

	// if (!(_userState & USERSTATE_IFACE_INVENTORY))	// Don't draw inventory unless active
	// 	return;
    graphics_clearInventory();

    uint8_t all = inventory_getCount(scummVars[VAR_EGO]);
	uint8_t max_inv = all - inventoryOffset;
	if (max_inv > 4)
	    max_inv = 4;
    uint8_t count = 0;
    uint8_t slot = 0;
    // Object *obj = objects;
    // Object *end = objects + _numObjects;
    Inventory *obj = invObjects;
    Inventory *end = invObjects + _numInventory;
    for ( ; obj != end ; ++obj)
    {
		if (obj->obj_nr == 0// || obj->whereis != WIO_INVENTORY
            || objectOwnerTable[obj->obj_nr] != scummVars[VAR_EGO])
			continue;

        ++count;
        if (count <= inventoryOffset)
            continue;

        char name[32];
        readName(name, obj->room, obj->OBCDoffset, obj->nameOffs);
        graphics_drawInventory(slot, name);
        ++slot;

        if (slot >= max_inv)
            break;

		//uint16_t obj = inventory_find(scummVars[VAR_EGO], i + 1 + inventoryOffset);

		// _string[1].ypos = _mouseOverBoxesV2[i].rect.top + vs->topline;
		// _string[1].xpos = _mouseOverBoxesV2[i].rect.left;
		// _string[1].right = _mouseOverBoxesV2[i].rect.right - 1;
		// _string[1].color = _mouseOverBoxesV2[i].color;

		// const byte *tmp = getObjOrActorName(obj);
		// assert(tmp);

		// Prevent inventory entries from overflowing by truncating the text
		// byte msg[20];
		// msg[maxChars] = 0;
		// strncpy((char *)msg, (const char *)tmp, maxChars);

		// Draw it
		// drawString(1, msg);
	}


	// If necessary, draw "up" arrow
	if (inventoryOffset > 0)
    {
        graphics_printAtXY("\x7e", INV_UP_X, INV_ARR_Y, 0, 3, 1);
	}

	// If necessary, draw "down" arrow
	if (inventoryOffset + 4 < all)
    {
        graphics_printAtXY("\x7f", INV_DOWN_X, INV_ARR_Y, 0, 3, 1);
	}
    EXIT;
}

void inventory_addObject(uint16_t id)
{
    ENTER;
	// int idx, slot;
	// uint32 size;
	// const byte *ptr;
	// byte *dst;
	// FindObjectInRoom foir;

	// debug(1, "Adding object %d from room %d into inventory", obj, room);

	// if (whereIsObject(obj) == WIO_FLOBJECT) {
	// 	idx = getObjectIndex(obj);
	// 	assert(idx >= 0);
	// 	ptr = getResourceAddress(rtFlObject, _objs[idx].fl_object_index) + 8;
	// 	assert(ptr);
	// 	size = READ_BE_UINT32(ptr + 4);
	// } else {
	// 	findObjectInRoom(&foir, foCodeHeader, obj, room);
	// 	if (_game.features & GF_OLD_BUNDLE)
	// 		size = READ_LE_UINT16(foir.obcd);
	// 	else if (_game.features & GF_SMALL_HEADER)
	// 		size = READ_LE_UINT32(foir.obcd);
	// 	else
	// 		size = READ_BE_UINT32(foir.obcd + 4);
	// 	ptr = foir.obcd;
	// }
    Object *obj = object_get(id);

	uint8_t slot = inventory_getSlot();
    invObjects[slot].obj_nr = obj->obj_nr;
    invObjects[slot].room = currentRoom;
    invObjects[slot].OBCDoffset = obj->OBCDoffset;
    invObjects[slot].nameOffs = obj->nameOffs;
	//inventory[slot] = obj->obj_nr;
	// dst = _res->createResource(rtInventory, slot, size);
	// assert(dst);
	// memcpy(dst, ptr, size);

    //obj->whereis = WIO_INVENTORY;
    EXIT;
}

uint16_t inventory_checkXY(int8_t x, int8_t y)
{
    //DEBUG_PRINTF("check %d %d\n", x, y);
    y -= INV_TOP;
    x -= INV_GAP;
    if (x < 0 || y < 0 || y >= INV_ROWS)
    {
        return 0;
    }

    if (x < INV_SLOT_WIDTH)
    {
        x = 0;
    }
    else if (x >= INV_SLOT_WIDTH + INV_ARR_W && x < INV_WIDTH)
    {
        x = 1;
    }
    else
    {
        return 0;
    }
    ENTER;
    //DEBUG_PRINTF(" --- %d %d\n", x, y);
    uint8_t slot = x + y * INV_COLS + inventoryOffset;
    uint8_t count = 0;
    Inventory *obj = invObjects;
    Inventory *end = invObjects + _numInventory;
    for ( ; obj != end ; ++obj)
    {
		if (obj->obj_nr == 0
            || objectOwnerTable[obj->obj_nr] != scummVars[VAR_EGO])
			continue;

        if (slot == count)
        {
            uint8_t res = obj->obj_nr;
            EXIT;
            return res;
        }

        ++count;
    }
    EXIT;

    return 0;
}

void inventory_checkButtons(int8_t x, int8_t y)
{
    ENTER;
    // check up and down arrows
    if (y == INV_ARR_Y)
    {
        DEBUG_PRINTF("Check buttons %d %d\n", x, inventoryOffset);
        if (x == INV_UP_X && inventoryOffset > 0)
        {
            inventoryOffset -= 2;
            inventory_redraw();
        }
        else if (x == INV_DOWN_X)
        {
            uint8_t all = inventory_getCount(scummVars[VAR_EGO]);
            if (all - inventoryOffset > INV_COLS * INV_ROWS)
            {
                inventoryOffset += 2;
                inventory_redraw();
            }
        }
    }
    EXIT;
}
