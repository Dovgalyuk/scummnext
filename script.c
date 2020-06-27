#include <arch/zxn/esxdos.h>
#include "script.h"
#include "resource.h"
#include "debug.h"
#include "actor.h"
#include "room.h"
#include "camera.h"
#include "object.h"
#include "graphics.h"

typedef struct Frame
{
    uint8_t room;
    uint16_t roomoffs;
    uint16_t offset;
} Frame;

#define STACK_SIZE 4

enum {
    PARAM_1 = 0x80,
    PARAM_2 = 0x40,
    PARAM_3 = 0x20
};

#define _numVariables 800

static Frame stack[STACK_SIZE];
uint16_t scummVars[_numVariables];
static int8_t curScript = -1;
static uint8_t scriptBytes[4096];
static uint16_t resultVarNumber;
static uint8_t opcode;
static uint8_t exitFlag;
static uint32_t delay;

static void loadScriptBytes(void)
{
    HROOM r = openRoom(stack[curScript].room);
    esx_f_seek(r, stack[curScript].roomoffs, ESX_SEEK_SET);
    readResource(r, scriptBytes, sizeof(scriptBytes));
    closeRoom(r);
    // for (int8_t i = 0 ; i < 16 ; ++i)
    //     DEBUG_PRINTF("%x ", scriptBytes[i]);
    // DEBUG_PUTS("\n");
}

static uint8_t fetchScriptByte(void)
{
    return scriptBytes[stack[curScript].offset++];
}

static uint16_t fetchScriptWord()
{
    uint8_t b1 = fetchScriptByte();
    uint8_t b2 = fetchScriptByte();
    return b1 + (b2 << 8);
}

static uint16_t readVar(uint16_t v)
{
    return scummVars[v];
}

static uint16_t getVar(void)
{
	return readVar(fetchScriptByte());
}

void writeVar(uint16_t var, uint16_t val)
{
    scummVars[var] = val;
    //DEBUG_PRINTF("var%u = %u\n", var, val);
}

static int16_t fetchScriptWordSigned(void)
{
    return (int16_t)fetchScriptWord();
}

static uint8_t getVarOrDirectByte(uint8_t mask)
{
	if (opcode & mask)
		return getVar();
	return fetchScriptByte();
}

static int16_t getVarOrDirectWord(uint8_t mask)
{
    if (opcode & mask)
        return getVar();
    return fetchScriptWordSigned();
}

static void setResult(uint16_t value)
{
	writeVar(resultVarNumber, value);
}

void stopScript(void)
{
    if (curScript == -1)
    {
        DEBUG_PUTS("Can't stop script, none is running\n");
        DEBUG_HALT;
    }
    DEBUG_PRINTF("Stopping the script %u/%u offset %x\n",
        stack[curScript].room, stack[curScript].roomoffs, stack[curScript].offset - 4);
    --curScript;
    if (curScript >= 0)
    {
        loadScriptBytes();
    }
}

static void getResultPos(void)
{
    resultVarNumber = fetchScriptByte();
}

static void pushScript(uint8_t room, uint16_t roomoffs, uint16_t offset)
{
    DEBUG_PRINTF("Starting script %u/%u\n", room, roomoffs);
    ++curScript;
    if (curScript >= STACK_SIZE - 1)
    {
        DEBUG_PUTS("Can't execute more scripts\n");
        DEBUG_HALT;
    }
    stack[curScript].room = room;
    stack[curScript].roomoffs = roomoffs;
    loadScriptBytes();
    stack[curScript].offset = offset;
}

static void parseString(uint8_t actor)
{
    uint8_t buffer[512];
    uint8_t *ptr = buffer;
	uint8_t c;

	while ((c = fetchScriptByte())) {

		uint8_t insertSpace = (c & 0x80) != 0;
		c &= 0x7f;

		// if (c < 8) {
		// 	// Special codes as seen in CHARSET_1 etc. My guess is that they
		// 	// have a similar function as the corresponding embedded stuff in modern
		// 	// games. Hence for now we convert them to the modern format.
		// 	// This might allow us to reuse the existing code.
		// 	// *ptr++ = 0xFF;
		// 	// *ptr++ = c;
		// 	if (c > 3) {
		// 		*ptr++ = fetchScriptByte();
		// 		*ptr++ = 0;
		// 	}
		// } else
		// *ptr++ = c;
        DEBUG_PUTC(c);
        *ptr++ = c;

		if (insertSpace)
        {
            DEBUG_PUTC(' ');
			*ptr++ = ' ';
        }

	}
	*ptr = 0;

    // TODO
	// int textSlot = 0;
	// _string[textSlot].xpos = 0;
	// _string[textSlot].ypos = 0;
	// _string[textSlot].right = _screenWidth - 1;
	// _string[textSlot].center = false;
	// _string[textSlot].overhead = false;

	actor_talk(actor, buffer);
}

static void jumpRelative(int cond)
{
	int16_t offset = fetchScriptWord();
	if (!cond)
		stack[curScript].offset += offset;
//    DEBUG_PRINTF("   jump to %x\n", stack[curScript].offset + offset);
}

static uint8_t isScriptRunning(uint8_t s)
{
    return 0;
}

static uint8_t getState(uint16_t obj)
{
    return 0;
}

static uint16_t getActiveObject(void)
{
	return getVarOrDirectWord(PARAM_1);
}

static void ifStateCommon(uint16_t type)
{
	uint8_t obj = getActiveObject();

	jumpRelative((getState(obj) & type) != 0);
}

///////////////////////////////////////////////////////////
// opcodes
///////////////////////////////////////////////////////////

static void op_putActor(void)
{
	uint8_t act = getVarOrDirectByte(PARAM_1);
	uint8_t x = getVarOrDirectByte(PARAM_2);
	uint8_t y = getVarOrDirectByte(PARAM_3);

	actor_put(act, x, y);
}

static void op_startMusic(void)
{
    getVarOrDirectByte(PARAM_1);
}

static void op_drawObject(void)
{
	uint8_t idx, i;
	// ObjectData *od;
	// uint16 x, y, w, h;
	uint8_t xpos, ypos;

	idx = getVarOrDirectWord(PARAM_1);
	xpos = getVarOrDirectByte(PARAM_2);
	ypos = getVarOrDirectByte(PARAM_3);
    DEBUG_PRINTF("Draw object %u at %u,%u\n", idx, xpos, ypos);

    Object *obj = object_get(idx);
    DEBUG_ASSERT(obj);

	if (xpos != 0xFF)
    {
        DEBUG_PUTS("TODO: drawing object at coordinates\n");
        DEBUG_HALT;
	// 	od->walk_x += (xpos * 8) - od->x_pos;
	// 	od->x_pos = xpos * 8;
	// 	od->walk_y += (ypos * 8) - od->y_pos;
	// 	od->y_pos = ypos * 8;
	}
	graphics_drawObject(obj);

	// x = od->x_pos;
	// y = od->y_pos;
	// w = od->width;
	// h = od->height;

	// i = _numLocalObjects;
	// while (i--) {
	// 	if (_objs[i].obj_nr && _objs[i].x_pos == x && _objs[i].y_pos == y && _objs[i].width == w && _objs[i].height == h)
	// 		putState(_objs[i].obj_nr, getState(_objs[i].obj_nr) & ~kObjectState_08);
	// }

	// putState(obj, getState(od->obj_nr) | kObjectState_08);
}

static void op_setState08(void)
{
    //DEBUG_PUTS("setState08\n");
	int obj = getVarOrDirectWord(PARAM_1);
	// putState(obj, getState(obj) | kObjectState_08);
	// markObjectRectAsDirty(obj);
	// clearDrawObjectQueue();
}

static void op_resourceRoutines(void)
{
    //DEBUG_PUTS("resourceRoutines\n");
	uint8_t resid = getVarOrDirectByte(PARAM_1);
	uint8_t opcode = fetchScriptByte();
    //DEBUG_PRINTF("   resid %u opcode %u\n", resid, opcode);
    // TODO
}

static void op_animateActor(void)
{
    DEBUG_PUTS("animateActor\n");
	uint8_t act = getVarOrDirectByte(PARAM_1);
	uint8_t anim = getVarOrDirectByte(PARAM_2);

	// Actor *a = derefActor(act, "o5_animateActor");
	// a->animateActor(anim);
}

static void op_panCameraTo(void)
{
    //DEBUG_PUTS("panCameraTo\n");
    camera_panTo(getVarOrDirectByte(PARAM_1));
}

static void op_move(void)
{
    //DEBUG_PUTS("move\n");
	getResultPos();
	setResult(getVarOrDirectWord(PARAM_1));
}

static void op_setBitVar(void)
{
	uint16_t var = fetchScriptWord();
	uint8_t a = getVarOrDirectByte(PARAM_1);

	uint16_t bit_var = var + a;
	uint8_t bit_offset = bit_var & 0x0f;
	bit_var >>= 4;

	if (getVarOrDirectByte(PARAM_2))
		scummVars[bit_var] |= (1 << bit_offset);
	else
		scummVars[bit_var] &= ~(1 << bit_offset);
}

static void op_startSound(void)
{
    //DEBUG_PUTS("startSound\n");
    getVarOrDirectByte(PARAM_1);
}

static void op_walkActorTo(void)
{
	uint8_t act = getVarOrDirectByte(PARAM_1);
	uint8_t x = getVarOrDirectByte(PARAM_2);
	uint8_t y = getVarOrDirectByte(PARAM_3);

    actor_startWalk(act, x, y);
}

static void op_putActorInRoom(void)
{
	uint8_t act = getVarOrDirectByte(PARAM_1);
    uint8_t room = getVarOrDirectByte(PARAM_2);
    actor_setRoom(act, room);
}

static void op_actorOps(void)
{
	uint8_t act = getVarOrDirectByte(PARAM_1);
	uint8_t arg = getVarOrDirectByte(PARAM_2);
    uint8_t i;

	uint8_t opcode = fetchScriptByte();
	if (act == 0 && opcode == 5) {
		// This case happens in the Zak/MM bootscripts, to set the default talk color (9).
		//_string[0].color = arg;
        DEBUG_PRINTF("TODO: Set default talk color %u\n", arg);
		return;
	}

	// a = derefActor(act, "actorOps");

	switch (opcode) {
	case 1:		// SO_SOUND
		//a->_sound[0] = arg;
        DEBUG_PRINTF("TODO: Set actor %u sound %u\n", act, arg);
		break;
	// case 2:		// SO_PALETTE
    //     i = fetchScriptByte();
	// 	//a->setPalette(i, arg);
    //     DEBUG_PRINTF("  TODO: Set actor %u palette %u/%u\n", act, i, arg);
	// 	break;
	case 3:		// SO_ACTOR_NAME
		//loadPtrToResource(rtActorName, a->_number, NULL);
        DEBUG_PRINTF("TODO: Set actor %u talk name '", act);
        {
            char c;
            do
            {
                c = fetchScriptByte();
                if (c)
                    DEBUG_PUTC(c);
            } while (c);
        }
        DEBUG_PUTS("'\n");
		break;
	case 4:		// SO_COSTUME
        actor_setCostume(act, arg);
		break;
	case 5:		// SO_TALK_COLOR
        //a->_talkColor = arg;
        DEBUG_PRINTF("TODO: Set actor %u talk color %u\n", act, arg);
		break;
	default:
		DEBUG_PRINTF("actorOps: opcode %u not yet supported\n", opcode);
        DEBUG_HALT;
	}
}

static void op_print(void)
{
    uint8_t act = getVarOrDirectByte(PARAM_1);
    DEBUG_PRINTF("print(%u, ", act);
    parseString(act);
    DEBUG_PUTS(")\n");
}

static void op_getRandomNr(void)
{
    //DEBUG_PUTS("getRandomNr\n");
	getResultPos();
	setResult(/*_rnd.getRandomNumber*/(getVarOrDirectByte(PARAM_1)));
}

static void op_jumpRelative(void)
{
    jumpRelative(0);
}

static void op_setVarRange(void)
{
	uint16_t a, b;

    //DEBUG_PUTS("setVarRange\n");

	getResultPos();
	a = fetchScriptByte();
	do {
		if (opcode & 0x80)
			b = fetchScriptWordSigned();
		else
			b = fetchScriptByte();

		setResult(b);
		resultVarNumber++;
	} while (--a);
}

static void op_verbOps(void)
{
    uint8_t verb = fetchScriptByte();
	uint8_t slot, state;

	switch (verb) {
	case 0:		// SO_DELETE_VERBS
		slot = getVarOrDirectByte(PARAM_1) + 1;
		// assert(0 < slot && slot < _numVerbs);
		// killVerb(slot);
		break;

	case 0xFF:	// Verb On/Off
		verb = fetchScriptByte();
		state = fetchScriptByte();
		//slot = getVerbSlot(verb, 0);
		// _verbs[slot].curmode = state;
		break;

	default: {	// New Verb
		int x = fetchScriptByte() * 8;
		int y = fetchScriptByte() * 8;
		slot = getVarOrDirectByte(PARAM_1) + 1;
		int prep = fetchScriptByte(); // Only used in V1?
		// V1 Maniac verbs are relative to the 'verb area' - under the sentence
        // x += 8;

		// VerbSlot *vs;
		// assert(0 < slot && slot < _numVerbs);

		// vs = &_verbs[slot];
		// vs->verbid = verb;
        // vs->color = 1;
        // vs->hicolor = 1;
        // vs->dimcolor = 1;
		// vs->type = kTextVerbType;
		// vs->charset_nr = _string[0]._default.charset;
		// vs->curmode = 1;
		// vs->saveid = 0;
		// vs->key = 0;
		// vs->center = 0;
		// vs->imgindex = 0;
		// vs->prep = prep;

		// vs->curRect.left = x;
		// vs->curRect.top = y;

        // static const char keyboard[] = {
        //         'q','w','e','r',
        //         'a','s','d','f',
        //         'z','x','c','v'
        //     };
        // if (1 <= slot && slot <= ARRAYSIZE(keyboard))
        //     vs->key = keyboard[slot - 1];
		// }

        // skip the name
        while (fetchScriptByte())
            ;
        }
		break;
	}

	// // Force redraw of the modified verb slot
	// drawVerb(slot, 0);
	// verbMouseOver(0);
}

static void op_breakHere(void)
{
    //DEBUG_PUTS("breakHere\n");
    //stopScript();
    exitFlag = 1;
}

static void op_delayVariable(void)
{
	delay = getVar();
	//vm.slot[_currentScript].status = ssPaused;
	op_breakHere();
}

static void op_assignVarByte(void)
{
	getResultPos();
	setResult(fetchScriptByte());
}

static void op_isNotEqual(void)
{
	uint16_t a = getVar();
	uint16_t b = getVarOrDirectWord(PARAM_1);
	jumpRelative(b != a);
}

static void op_delay(void)
{
    uint8_t a = fetchScriptByte() ^ 0xff;
    uint8_t b = fetchScriptByte() ^ 0xff;
    uint8_t c = fetchScriptByte() ^ 0xff;
    //DEBUG_PRINTF("delay %x %x %x\n", a, b, c);
    delay = a | (b << 8) | ((uint32_t)c << 16);
	// vm.slot[_currentScript].status = ssPaused;
    op_breakHere();
}

static void op_setCameraAt(void)
{
    camera_setX(getVarOrDirectByte(PARAM_1)/* * V12_X_MULTIPLIER*/);
}

static void op_isLessEqual(void)
{
	uint16_t a = getVar();
	uint16_t b = getVarOrDirectWord(PARAM_1);
	jumpRelative(b <= a);
}

static void op_waitForActor(void)
{
    uint8_t act = getVarOrDirectByte(PARAM_1);
	if (actor_isMoving(act)) {
		stack[curScript].offset -= 2;
		op_breakHere();
	}
}

static void op_stopSound(void)
{
    //DEBUG_PUTS("stopSound\n");
    getVarOrDirectByte(PARAM_1);
}

static void op_cutscene(void)
{
    DEBUG_PUTS("cutscene\n");
    // TODO
}

static void op_startScript(void)
{
    //DEBUG_PUTS("startScript\n");
    uint8_t script = getVarOrDirectByte(PARAM_1);

    // TODO
    // if (script == MM_SCRIPT(82)) {
    //     if (isScriptRunning(MM_SCRIPT(83)) || isScriptRunning(MM_SCRIPT(84)))
    //         return;
    // }
    // if (_game.version >= 1 && script == 155) {
    //     if (VAR(120) == 1)
    //         return;
    // }
    DEBUG_PRINTF("New script id %u\n", script);
    pushScript(scripts[script].room, scripts[script].roomoffs, 4);
}

static void op_getActorX(void)
{
//    DEBUG_PUTS("getActorX\n");
	getResultPos();
    setResult(actor_getX(getVarOrDirectByte(PARAM_1)));
}

static void op_isEqual(void)
{
    uint8_t var = fetchScriptByte();
	uint16_t a = readVar(var);
	uint16_t b = getVarOrDirectWord(PARAM_1);
	jumpRelative(b == a);
}

static void op_actorFollowCamera(void)
{
    camera_followActor(getVarOrDirectByte(0x80));
}

static void op_beginOverride(void)
{
    //DEBUG_PUTS("beginOverride\n");
	// vm.cutScenePtr[0] = _scriptPointer - _scriptOrgPointer;
	// vm.cutSceneScript[0] = _currentScript;

	// Skip the jump instruction following the override instruction
	fetchScriptByte();
	fetchScriptWord();
}

static void op_add(void)
{
	uint16_t a;
	getResultPos();
	a = getVarOrDirectWord(PARAM_1);
	scummVars[resultVarNumber] += a;
}

static void op_cursorCommand(void)
{
	uint16_t cmd = getVarOrDirectWord(PARAM_1);
	uint8_t state = cmd >> 8;

	if (cmd & 0xFF) {
		scummVars[VAR_CURSORSTATE] = cmd & 0xFF;
	}

    // TODO
	// setUserState(state);
}

static void op_loadRoomWithEgo(void)
{
    // TODO?
	// Actor *a;
	// int obj, room, x, y, x2, y2, dir;

	uint16_t obj = getVarOrDirectWord(PARAM_1);
	uint8_t room = getVarOrDirectByte(PARAM_2);

	// a = derefActor(VAR(VAR_EGO), "o2_loadRoomWithEgo");

	// // The original interpreter sets the actors new room X/Y to the last rooms X/Y
	// // This fixes a problem with MM: script 161 in room 12, the 'Oomph!' script
	// // This scripts runs before the actor position is set to the correct room entry location
	// if ((_game.id == GID_MANIAC) && (_game.platform != Common::kPlatformNES)) {
	// 	a->putActor(a->getPos().x, a->getPos().y, room);
	// } else {
	// 	a->putActor(0, 0, room);
	// }
	// _egoPositioned = false;

	int8_t x = fetchScriptByte();
	int8_t y = fetchScriptByte();

	// startScene(a->_room, a, obj);

	// getObjectXYPos(obj, x2, y2, dir);
	// AdjustBoxResult r = a->adjustXYToBeInBox(x2, y2);
	// x2 = r.x;
	// y2 = r.y;
	// a->putActor(x2, y2, _currentRoom);
	// a->setDirection(dir + 180);

	// camera._dest.x = camera._cur.x = a->getPos().x;
	// setCameraAt(a->getPos().x, a->getPos().y);
	// setCameraFollows(a);

	// _fullRedraw = true;

	// resetSentence();

	// if (x >= 0 && y >= 0) {
	// 	a->startWalkActor(x, y, -1);
	// }
	runScript(5);
}

static void op_isScriptRunning(void)
{
//    DEBUG_PUTS("isScriptRunning\n");
	getResultPos();
	setResult(isScriptRunning(getVarOrDirectByte(PARAM_1)));
}

static void op_lights(void)
{
	uint8_t a, b, c;

	a = getVarOrDirectByte(PARAM_1);
	b = fetchScriptByte();
	c = fetchScriptByte();

    DEBUG_PRINTF("Lights %u %u %u\n", a, b, c);

	if (c == 0)
    {
		scummVars[VAR_CURRENT_LIGHTS] = a;
	}
    else if (c == 1)
    {
		//_flashlight.xStrips = a;
		//_flashlight.yStrips = b;
	}
}

static void op_loadRoom(void)
{
    uint8_t room = getVarOrDirectByte(PARAM_1);
    DEBUG_PRINTF("loadRoom %u\n", room);

    startScene(room);
    // _fullRedraw = true;
}

static void op_isGreater(void)
{
//    DEBUG_PUTS("isGreater");
	uint16_t a = getVar();
	uint16_t b = getVarOrDirectWord(PARAM_1);
//    DEBUG_PRINTF("%u %u\n", a, b);
	jumpRelative(b > a);
}

static void op_switchCostumeSet(void)
{
    graphics_loadCostumeSet(fetchScriptByte());
}

static void op_findObject(void)
{
//    DEBUG_PUTS("findObject\n");
	uint16_t obj = 0;
	getResultPos();
	uint16_t x = getVarOrDirectByte(PARAM_1) * V12_X_MULTIPLIER;
	uint16_t y = getVarOrDirectByte(PARAM_2) * V12_Y_MULTIPLIER;
	// obj = findObject(x, y);
	// if (obj == 0 && (_game.platform == Common::kPlatformNES) && (_userState & USERSTATE_IFACE_INVENTORY)) {
	// 	if (_mouseOverBoxV2 >= 0 && _mouseOverBoxV2 < 4)
	// 		obj = findInventory(VAR(VAR_EGO), _mouseOverBoxV2 + _inventoryOffset + 1);
	// }
	setResult(obj);
}

static void op_ifState01(void)
{
    ifStateCommon(kObjectStatePickupable);
}

void executeScript(void)
{
    if (delay)
    {
        //DEBUG_PRINTF("Delay %u\n", (uint16_t)delay);
        // if (delay > 100)
        //     delay -= 100;
        // else
        //     delay = 0;
        --delay;
        return;
    }
    while (curScript >= 0 && !exitFlag)
    {
        //DEBUG_PRINTF("EXEC %u %x\n", stack[curScript].id, stack[curScript].offset - 4);
        opcode = fetchScriptByte();
        switch (opcode)
        {
        case 0x00:
            stopScript();
            break;
        case 0x01:
            op_putActor();
            break;
        case 0x02:
            op_startMusic();
            break;
        case 0x05:
            op_drawObject();
            break;
        case 0x07:
            op_setState08();
            break;
        case 0x0c:
            op_resourceRoutines();
            break;
        case 0x11:
            op_animateActor();
            break;
        case 0x12:
            op_panCameraTo();
            break;
        case 0x13:
        case 0x53:
            op_actorOps();
            break;
        case 0x14:
            op_print();
            break;
        case 0x16:
            op_getRandomNr();
            break;
        case 0x18:
            op_jumpRelative();
            break;
        case 0x1a:
            op_move();
            break;
        case 0x1b:
        case 0x5b:
            op_setBitVar();
            break;
        case 0x1c:
            op_startSound();
            break;
        case 0x1e:
            op_walkActorTo();
            break;
        // case 0x26:
        //     op_setVarRange();
        //     break;
        case 0x2b:
            op_delayVariable();
            break;
        case 0x2c:
            op_assignVarByte();
            break;
        case 0x2d:
            op_putActorInRoom();
            break;
        case 0x2e:
            op_delay();
            break;
        case 0x32:
            op_setCameraAt();
            break;
        case 0x38:
            op_isLessEqual();
            break;
        case 0x3b:
            op_waitForActor();
            break;
        case 0x3c:
            op_stopSound();
            break;
        case 0x40:
            op_cutscene();
            break;
        case 0x42:
            op_startScript();
            break;
        case 0x43:
            op_getActorX();
            break;
        case 0x48:
            op_isEqual();
            break;
        case 0x52:
            op_actorFollowCamera();
            break;
        case 0x58:
            op_beginOverride();
            break;
        case 0x5a:
            op_add();
            break;
        case 0x60:
            op_cursorCommand();
            break;
        case 0x62:
            stopScript();
            break;
        // case 0x64:
        //     op_loadRoomWithEgo();
        //     break;
        case 0x68:
            op_isScriptRunning();
            break;
        case 0x70:
            op_lights();
            break;
        case 0x72:
            op_loadRoom();
            break;
        case 0x78:
            op_isGreater();
            break;
        case 0x7a:
            op_verbOps();
            break;
        case 0x80:
            op_breakHere();
            break;
        case 0x88:
            op_isNotEqual();
            break;
        case 0xa0:
            stopScript();
            break;
        case 0xab:
            op_switchCostumeSet();
            break;
        case 0xf5:
            op_findObject();
            break;
        case 0xff:
            op_ifState01();
            break;
        default:
            DEBUG_PRINTF("Unknown opcode %x\n", opcode);
            stopScript();
            break;
        }
    }
}

void runScript(uint8_t s)
{
    DEBUG_PRINTF("New script id %u\n", s);
    pushScript(scripts[s].room, scripts[s].roomoffs, 4);
    executeScript();
}

void processScript(void)
{
    exitFlag = 0;
    executeScript();
}

void runRoomScript(uint8_t room, uint16_t offs)
{
    pushScript(room, offs, 0);
}
