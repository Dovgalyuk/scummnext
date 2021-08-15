#include <arch/zxn.h>
#include <arch/zxn/esxdos.h>
#include "debug.h"
#include "resource.h"
#include "script.h"
#include "graphics.h"
#include "camera.h"
#include "actor.h"
#include "cursor.h"
#include "object.h"
#include "sprites.h"
#include "string.h"
#include "costume.h"

const int _numRooms = 55;

// costumes 25-36 are special. see v1MMNEScostTables[] in costume.cpp
// costumes 37-76 are room graphics resources
// costume 77 is a character set translation table
// costume 78 is a preposition list
// costume 79 is unused but allocated, so the total is a nice even number :)

int _numBitVariables = 4096;			// 2048
int _numArray = 50;
int _numNewNames = 50;
int _numCharsets = 9;					// 9
// all scripts are global in MM
//int _numGlobalScripts = 200;
int _numFlObject = 50;

int _shadowPaletteSize = 256;

int main()
{
    ZXN_NEXTREGA(REG_MACHINE_TYPE, RMT_TIMING_P3E | RMT_P3E);
    ZXN_NEXTREG(REG_TURBO_MODE, 3/*RTM_14MHZ*/); // 28MHz

    // init memory bank 0-16k
    ZXN_WRITE_MMU0(3);
    //ZXN_WRITE_MMU1(32);

    int i;
    HROOM f = openRoom(0);

    uint16_t magic = readWord(f);
	//DEBUG_PRINTF("Magic %x\n", magic); /* version magic number */
    readGlobalObjects(f);

    // room offsets are always 0, skip them
    // 3 - roomno + roomoffs size
    esx_f_seek(f, _numRooms * 3, ESX_SEEK_FWD);

	for (i = 0; i < _numCostumes; i++) {
        costumes[i].room = readByte(f);
	}
	for (i = 0; i < _numCostumes; i++) {
        costumes[i].roomoffs = readWord(f);
        // HROOM c = seekResource(&costumes[i]);
        // uint16_t sz = readWord(c);
        // closeRoom(c);
        // DEBUG_PRINTF("costume %u room %u offs %u size %u\n", i, costumes[i].room, costumes[i].roomoffs, sz);
	}

	for (i = 0; i < _numScripts; i++) {
        scripts[i].room = readByte(f);
	}
	for (i = 0; i < _numScripts; i++) {
        scripts[i].roomoffs = readWord(f);
        //DEBUG_PRINTF("script %u room %u offs %x\n", i, scripts[i].room, scripts[i].roomoffs);
	}

    closeRoom(f);

    costume_loadData();

    initGraphics();

    // NES base tiles
    decodeTiles(0);

    // boot script
    runScript(1);

    while (1)
    {
        actors_animate();
        actors_walk();
        camera_move();
        messages_update();
        updateScummVars();
        processScript();
        checkExecVerbs();
        checkAndRunSentenceScript();
        cursor_animate();

        graphics_updateScreen();

        //DEBUG_DELAY(5000);
    }
}
