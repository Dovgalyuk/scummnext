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
static uint16_t entryScriptOffs;

static void setupRoomSubBlocks(void)
{
    HROOM r = openRoom(currentRoom);
    uint16_t size = readWord(r);
    readWord(r);
    roomWidth = readWord(r);
    //uint16_t height = readWord(r) * 8;
    esx_f_seek(r, 20, ESX_SEEK_SET);
    uint8_t numObj = readByte(r);
    uint8_t matrixOffs = readByte(r);
    uint8_t numSnd = readByte(r);
    uint8_t numScript = readByte(r);
    uint16_t exitOffs = readWord(r);
    entryScriptOffs = readWord(r);
    uint16_t exitLen = entryScriptOffs - exitOffs;// + 4;
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
    DEBUG_ASSERT(numBoxes <= MAX_BOXES);
    readBuffer(r, (uint8_t*)boxes, sizeof(Box) * numBoxes);
    // boxes matrix
    readBuffer(r, boxesMatrix, numBoxes * (numBoxes + 1));

    DEBUG_PRINTF("Open room %u width %u objects %u boxes %u\n", currentRoom, roomWidth, numObj, numBoxes);

    int i, j;
    for (i = 0 ; i < numBoxes ; ++i)
    {
        DEBUG_PRINTF("Box %d uy=%d ly=%d ulx=%d urx=%d llx=%d lrx=%d mask=%x flags=%x\n",
            i, boxes[i].uy, boxes[i].ly, boxes[i].ulx, boxes[i].urx,
            boxes[i].llx, boxes[i].lrx, boxes[i].mask, boxes[i].flags
        );
    }
    uint8_t *ptr = boxesMatrix;
    for (i = 0 ; i < numBoxes + 1 ; ++i)
    {
        for (j = 0 ; j < numBoxes ; ++j)
            DEBUG_PRINTF("%d ", *ptr++);
        DEBUG_PUTS("\n");
    }

    decodeNESGfx(r);
    setupRoomObjects(r);

    closeRoom(r);
}

void startScene(uint8_t room/*, Actor *a, int objectNr*/)
{
	//int i, where;

	// debugC(DEBUG_GENERAL, "Loading room %d", room);

	// stopTalk();

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

	// if (VAR_NEW_ROOM != 0xFF)
	// 	VAR(VAR_NEW_ROOM) = room;

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

	// VAR(VAR_ROOM) = room;
	// _fullRedraw = true;

	// _res->increaseResourceCounters();

	currentRoom = room;
	// VAR(VAR_ROOM) = room;

    // _roomResource = room;

	// if (VAR_ROOM_RESOURCE != 0xFF)
	// 	VAR(VAR_ROOM_RESOURCE) = _roomResource;

	// if (room != 0)
	// 	ensureResourceLoaded(rtRoom, room);

	// clearRoomObjects();

	// if (_currentRoom == 0) {
	// 	_ENCD_offs = _EXCD_offs = 0;
	// 	_numObjectsInRoom = 0;
	// 	return;
	// }

	setupRoomSubBlocks();
	// resetRoomSubBlocks();

	// initBGBuffers(_roomHeight);

	//resetRoomObjects();

	// if (VAR_ROOM_WIDTH != 0xFF && VAR_ROOM_HEIGHT != 0xFF) {
	// 	VAR(VAR_ROOM_WIDTH) = _roomWidth;
	// 	VAR(VAR_ROOM_HEIGHT) = _roomHeight;
	// }

	// if (VAR_CAMERA_MIN_X != 0xFF)
	// 	VAR(VAR_CAMERA_MIN_X) = _screenWidth / 2;
	// if (VAR_CAMERA_MAX_X != 0xFF)
	// 	VAR(VAR_CAMERA_MAX_X) = _roomWidth - (_screenWidth / 2);

    camera_setX(roomWidth / 2);
    // camera._mode = kNormalCameraMode;
    // camera._cur.y = camera._dest.y = _screenHeight / 2;

	// if (_roomResource == 0)
	// 	return;

	// memset(gfxUsageBits, 0, sizeof(gfxUsageBits));

	// showActors();

	// _egoPositioned = false;

    if (entryScriptOffs)
    {
        // runEntryScript();
        runRoomScript(room, entryScriptOffs);
    }
    // runScript(5, 0, 0, 0);

	// _doEffect = true;
}
