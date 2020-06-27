#include <arch/zxn.h>
#include <arch/zxn/esxdos.h>
#include "debug.h"
#include "resource.h"
#include "script.h"
#include "graphics.h"
#include "camera.h"
#include "actor.h"

const int _numGlobalObjects = 775;
const int _numRooms = 55;

static const int v1MMNEScostTables[2][6] = {
	/* desc lens offs data  gfx  pal */
	{ 25,  27,  29,  31,  33,  35},
	{ 26,  28,  30,  32,  34,  36}
};

// costumes 25-36 are special. see v1MMNEScostTables[] in costume.cpp
// costumes 37-76 are room graphics resources
// costume 77 is a character set translation table
// costume 78 is a preposition list
// costume 79 is unused but allocated, so the total is a nice even number :)

int _numBitVariables = 4096;			// 2048
int _numArray = 50;
int _numVerbs = 100;
int _numNewNames = 50;
int _numCharsets = 9;					// 9
int _numInventory = 80;					// 80
// all scripts are global in MM
//int _numGlobalScripts = 200;
int _numFlObject = 50;

int _shadowPaletteSize = 256;

uint16_t getWord(uint8_t *p)
{
    return p[0] + p[1] * 0x100;
}

// not for sound
// uint8_t *loadResource(Resource *res)
// {
//     // if (res.resource)
//     //     return res.resource;
//     HFILE f = openRoom(res->room);
//     esx_f_seek(f, res->roomoffs, ESX_SEEK_SET);

//     // old bundle
//     uint16_t size = readWord(f);
//     esx_f_seek(f, 2, ESX_SEEK_BWD);

//     // read buffer
//     uint8_t *buf = malloc(size);
//     esx_f_read(f, buf, size);

//     closeRoom(f);

//     return buf;
// }

// uint8_t *loadCostume(int n)
// {
//     return loadResource(&costumes[n]);
// }

int main()
{
    int i;
    HROOM f = openRoom(0);

    uint16_t magic = readWord(f);
	DEBUG_PRINTF("Magic %x\n", magic); /* version magic number */
	for (i = 0; i != _numGlobalObjects; i++) {
	 	uint8_t tmp = readByte(f);
        //printf("global %u owner %u state %u\n", i, tmp & 15, tmp >> 4);

	// 	_objectOwnerTable[i] = tmp & OF_OWNER_MASK;
	// 	_objectStateTable[i] = tmp >> OF_STATE_SHL;
	}

    // room offsets are always 0, skip them
    // 3 - roomno + roomoffs size
    esx_f_seek(f, _numRooms * 3, ESX_SEEK_FWD);

	for (i = 0; i < _numCostumes; i++) {
        costumes[i].room = readByte(f);
	}
	for (i = 0; i < _numCostumes; i++) {
        costumes[i].roomoffs = readWord(f);
        //printf("costume %u room %u offs %u\n", i, costumes[i].room, costumes[i].roomoffs);
	}

	for (i = 0; i < _numScripts; i++) {
        scripts[i].room = readByte(f);
	}
	for (i = 0; i < _numScripts; i++) {
        scripts[i].roomoffs = readWord(f);
        //DEBUG_PRINTF("script %u room %u offs %x\n", i, scripts[i].room, scripts[i].roomoffs);
	}

    closeRoom(f);

    // decodeNESBaseTiles();
    // while (1);

    initGraphics();

    // NES charset
    decodeNESTrTable();
    decodeNESBaseTiles();

    // boot script
    runScript(1);

    while (1)
    {
        actors_walk();
        camera_move();
        processScript();

        graphics_updateScreen();
    }
}
