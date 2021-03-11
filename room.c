#include <arch/zxn/esxdos.h>
#include "debug.h"
#include "resource.h"
#include "graphics.h"
#include "camera.h"
#include "object.h"
#include "script.h"
#include "box.h"

uint8_t currentRoom;
uint8_t roomWidth;
uint8_t roomHeight;
static uint16_t entryScriptOffs;
static uint16_t exitScriptOffs;

static void setupRoomSubBlocks(void)
{
    HROOM r = openRoom(currentRoom);
    uint16_t size = readWord(r);
    readWord(r);
    roomWidth = readWord(r); // *8?
    roomHeight = readWord(r); // *8 ?
    esx_f_seek(r, 20, ESX_SEEK_SET);
    uint8_t numObj = readByte(r);
    uint8_t matrixOffs = readByte(r);
    uint8_t numSnd = readByte(r);
    uint8_t numScript = readByte(r);
    exitScriptOffs = readWord(r);
    entryScriptOffs = readWord(r);
    uint16_t exitLen = entryScriptOffs - exitScriptOffs;// + 4;
    uint16_t entryLen = size - entryScriptOffs;// + 4;
    // offset 28 - objects start here

    // then sound and scripts
    // ptr = roomptr + 28 + num_objects * 4;
    // while (num_sounds--)
    // 	loadResource(rtSound, *ptr++);
    // while (num_scripts--)
    // 	loadResource(rtScript, *ptr++);

    esx_f_seek(r, matrixOffs, ESX_SEEK_SET);
    // boxes
    numBoxes = readByte(r);
    DEBUG_ASSERT(numBoxes <= MAX_BOXES, "setupRoomSubBlocks");
    readBuffer(r, (uint8_t*)boxes, sizeof(Box) * numBoxes);
    // boxes matrix
    readBuffer(r, boxesMatrix, numBoxes * (numBoxes + 1));

    DEBUG_PRINTF("Open room %u width %u objects %u boxes %u\n", currentRoom, roomWidth, numObj, numBoxes);

    // int i;
    // for (i = 0 ; i < numBoxes ; ++i)
    // {
    //     DEBUG_PRINTF("Box %d uy=%d ly=%d ulx=%d urx=%d llx=%d lrx=%d mask=%x flags=%x\n",
    //         i, boxes[i].uy, boxes[i].ly, boxes[i].ulx, boxes[i].urx,
    //         boxes[i].llx, boxes[i].lrx, boxes[i].mask, boxes[i].flags
    //     );
    // }
    // int j;
    // uint8_t *ptr = boxesMatrix;
    // for (i = 0 ; i < numBoxes + 1 ; ++i)
    // {
    //     for (j = 0 ; j < numBoxes ; ++j)
    //         DEBUG_PRINTF("%d ", *ptr++);
    //     DEBUG_PUTS("\n");
    // }

    decodeNESGfx(r);
    setupRoomObjects(r);

    closeRoom(r);
}

void startScene(uint8_t room/*, Actor *a, int objectNr*/)
{
	//int i, where;

	// debugC(DEBUG_GENERAL, "Loading room %d", room);

	actor_stopTalk();

	// fadeOut(_switchRoomEffect2);
	// _newEffect = _switchRoomEffect;

	// ScriptSlot *ss = &vm.slot[_currentScript];

	// if (_currentScript != 0xFF) {
	// 	if (ss->where == WIO_ROOM || ss->where == WIO_FLOBJECT) {
	// 		if (ss->cutsceneOverride && _game.version >= 5)
	// 			error("Object %d stopped with active cutscene/override in exit", ss->number);

	// 		nukeArrays(_currentScript);
	// 		_currentScript = 0xFF;
	// 	} else if (ss->where == WIO_LOCAL) {
	// 		if (ss->cutsceneOverride && _game.version >= 5)
	// 			error("Script %d stopped with active cutscene/override in exit", ss->number);

	// 		nukeArrays(_currentScript);
	// 		_currentScript = 0xFF;
	// 	}
	// }

	// killScriptsAndResources();

	// clearDrawQueues();

	// for (i = 1; i < _numActors; i++) {
	// 	_actors[i]->hideActor();
	// }

    // for (i = 0; i < 256; i++) {
    //     _roomPalette[i] = i;
    //     if (_shadowPalette)
    //         _shadowPalette[i] = i;
    // }
    // setDirtyColors(0, 255);

	// _fullRedraw = true;

	// _res->increaseResourceCounters();

	scummVars[VAR_ROOM] = room;

    if (exitScriptOffs)
    {
        runRoomScript(-1, currentRoom, exitScriptOffs);
    }

    // clear before changing the room id
	objects_clear();

	currentRoom = room;

    // _roomResource = room;

	scummVars[VAR_ROOM_RESOURCE] = room;

	// if (room != 0)
	// 	ensureResourceLoaded(rtRoom, room);

	if (currentRoom == 0)
    {
	// 	_numObjectsInRoom = 0;
        exitScriptOffs = 0;
        entryScriptOffs = 0;
	 	return;
	}

	setupRoomSubBlocks();
	// resetRoomSubBlocks();

	// initBGBuffers(_roomHeight);

	//resetRoomObjects();

	scummVars[VAR_CAMERA_MIN_X] = SCREEN_WIDTH / 2;
    scummVars[VAR_CAMERA_MAX_X] = roomWidth - SCREEN_WIDTH / 2;

    camera_setX(roomWidth / 2);
    // camera._mode = kNormalCameraMode;
    // camera._cur.y = camera._dest.y = _screenHeight / 2;

	// if (_roomResource == 0)
	// 	return;

	// memset(gfxUsageBits, 0, sizeof(gfxUsageBits));

	actors_show();

	// _egoPositioned = false;

    if (entryScriptOffs)
    {
        runRoomScript(-1, room, entryScriptOffs);
    }

    // this is for common rooms, should be run in parallel
    runScript(5);

	// _doEffect = true;
}
