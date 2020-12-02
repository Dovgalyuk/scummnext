#include <arch/zxn.h>
#include <arch/zxn/esxdos.h>
#include "script.h"
#include "resource.h"
#include "debug.h"
#include "actor.h"
#include "room.h"
#include "camera.h"
#include "object.h"
#include "graphics.h"
#include "cursor.h"
#include "verbs.h"

typedef struct Frame
{
    uint8_t id;
    uint8_t room;
    uint16_t roomoffs;
    uint16_t offset;
    uint32_t delay;
    uint16_t resultVarNumber;
} Frame;

#define MAX_SCRIPTS 8

enum {
    PARAM_1 = 0x80,
    PARAM_2 = 0x40,
    PARAM_3 = 0x20
};

enum UserStates {
	USERSTATE_SET_FREEZE      = 0x01,   // freeze scripts if USERSTATE_FREEZE_ON is set, unfreeze otherwise
	USERSTATE_SET_CURSOR      = 0x02,   // shows cursor if USERSTATE_CURSOR_ON is set, hides it otherwise
	USERSTATE_SET_IFACE       = 0x04,   // change user-interface (sentence-line, inventory, verb-area)
	USERSTATE_FREEZE_ON       = 0x08,   // only interpreted if USERSTATE_SET_FREEZE is set
	USERSTATE_CURSOR_ON       = 0x10,   // only interpreted if USERSTATE_SET_CURSOR is set
	USERSTATE_IFACE_SENTENCE  = 0x20,   // only interpreted if USERSTATE_SET_IFACE is set
	USERSTATE_IFACE_INVENTORY = 0x40,   // only interpreted if USERSTATE_SET_IFACE is set
	USERSTATE_IFACE_VERBS     = 0x80    // only interpreted if USERSTATE_SET_IFACE is set
};

#define _numVariables 800

static Frame stack[MAX_SCRIPTS];
int16_t scummVars[_numVariables];
static uint8_t curScript;
static uint8_t opcode;
static uint8_t exitFlag;

// mapped in MMU1 to pages 32-...
// can be switched for different scripts
extern uint8_t scriptBytes[4096];

#define FIRST_BANK 32

/////////////////////////////////////////////////////////////////////////
// scripting internals
/////////////////////////////////////////////////////////////////////////

static void loadScriptBytes(uint8_t s)
{
    HROOM r = openRoom(stack[s].room);
    esx_f_seek(r, stack[s].roomoffs, ESX_SEEK_SET);
    readResource(r, scriptBytes, sizeof(scriptBytes));
    closeRoom(r);
    // for (int8_t i = 0 ; i < 16 ; ++i)
    //     DEBUG_PRINTF("%x ", scriptBytes[i]);
    // DEBUG_PUTS("\n");
}

static uint8_t findEmptyFrame(void)
{
    uint8_t f;
    for (f = 0 ; f < MAX_SCRIPTS ; ++f)
        if (stack[f].id == 0)
            return f;
    DEBUG_ASSERT(0, "Number of scripts exceeded\n");
}

static void switchScriptPage(uint8_t s)
{
    //DEBUG_PRINTF("Select page %u\n", FIRST_BANK + s);
    ZXN_WRITE_MMU1(FIRST_BANK + s);
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
	writeVar(stack[curScript].resultVarNumber, value);
}

static void stopScript(uint8_t index)
{
    DEBUG_PRINTF("Stopping the script %u room %u/%u offset %x\n",
        stack[index].id, stack[index].room, stack[index].roomoffs,
        stack[index].offset - 4);
    stack[index].id = 0;
    if (index == curScript)
        exitFlag = 1;
}

static void op_stopObjectCode(void)
{
    stopScript(curScript);
}

static void op_stopScript(void)
{
    uint8_t s = getVarOrDirectByte(PARAM_1);

    DEBUG_PRINTF("Stopping from script %u room %u/%u next offset %x\n",
        stack[curScript].id, stack[curScript].room, stack[curScript].roomoffs,
        stack[curScript].offset);

    // WORKAROUND bug #4112: If you enter the lab while Dr. Fred has the powered turned off
    // to repair the Zom-B-Matic, the script will be stopped and the power will never turn
    // back on. This fix forces the power on, when the player enters the lab,
    // if the script which turned it off is running
    // if (_game.id == GID_MANIAC && _roomResource == 4 && isScriptRunning(MM_SCRIPT(138))) {
        // TODO

    uint8_t i = curScript;
    if (s != 0)
    {
        for (i = 0 ; i < MAX_SCRIPTS ; ++i)
            if (stack[i].id == s)
                break;
        if (i == MAX_SCRIPTS)
        {
            DEBUG_PRINTF("Stopping the script %u which is not executing\n", s);
            return;
        }
    }

    stopScript(i);
}

static void getResultPos(void)
{
    stack[curScript].resultVarNumber = fetchScriptByte();
}

static void pushScript(uint8_t id, uint8_t room, uint16_t roomoffs, uint16_t offset)
{
    uint8_t s = findEmptyFrame();
    DEBUG_PRINTF("Starting script %u room %u offset %u in frame %u\n", id, room, roomoffs, s);
    stack[s].id = id;
    stack[s].room = room;
    stack[s].roomoffs = roomoffs;
    stack[s].offset = offset;
    switchScriptPage(s);
    loadScriptBytes(s);
    switchScriptPage(curScript);
}

static void parseString(uint8_t actor)
{
    uint8_t buffer[64];
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
    uint8_t i;
    for (i = 0 ; i < MAX_SCRIPTS ; ++i)
        if (stack[i].id == s)
            return 1;
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

static uint8_t strcopy(char *d, const char *s)
{
    char *d0 = d;
    while (*d++ = *s++)
        ;
    // returns length in characters
    return d - d0 - 1;
}

static uint8_t getObjOrActorName(char *s, uint8_t id)
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

static void op_getActorRoom(void)
{
	getResultPos();
	uint8_t act = getVarOrDirectByte(PARAM_1);
	setResult(actors[act].room);
}

static void op_drawObject(void)
{
	uint8_t i;
	// ObjectData *od;
	// uint16 x, y, w, h;
	uint8_t xpos, ypos;
    uint16_t idx;

	idx = getVarOrDirectWord(PARAM_1);
	xpos = getVarOrDirectByte(PARAM_2);
	ypos = getVarOrDirectByte(PARAM_3);
    DEBUG_PRINTF("Draw object %u at %u,%u\n", idx, xpos, ypos);

    Object *obj = object_get(idx);
    DEBUG_ASSERT(obj, "op_drawObject");

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
 	uint16_t j = getVarOrDirectWord(PARAM_1);
    Object *obj = object_get(j);
    //DEBUG_ASSERT(obj, "op_setState08");
    DEBUG_PRINTF("setState08 %u\n", obj->obj_nr);

    obj->state |= kObjectState_08;
	graphics_drawObject(obj);

	// putState(obj, getState(obj) | kObjectState_08);
	// markObjectRectAsDirty(obj);
	// clearDrawObjectQueue();
}

static void op_clearState08(void)
{
    uint16_t j = getActiveObject();
    Object *obj = object_get(j);
    DEBUG_PRINTF("clearState08 %u\n", obj->obj_nr);
    obj->state &= ~kObjectState_08;
    //putState(obj, getState(obj) & ~kObjectState_08);
    //markObjectRectAsDirty(obj);
    //clearDrawObjectQueue();

	graphics_drawObject(obj);
}

static void op_resourceRoutines(void)
{
    //DEBUG_PUTS("resourceRoutines\n");
	uint8_t resid = getVarOrDirectByte(PARAM_1);
	uint8_t opcode = fetchScriptByte();
    // DEBUG_PRINTF("   resid %u opcode %u\n", resid, opcode);
    // just ensures that resource is loaded
    // TODO
}

static void op_animateActor(void)
{
	uint8_t act = getVarOrDirectByte(PARAM_1);
	uint8_t anim = getVarOrDirectByte(PARAM_2);
    DEBUG_PRINTF("animateActor %u %u\n", act, anim);

	actor_animate(act, anim);
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

    Actor *a = &actors[act];

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
        DEBUG_PRINTF("Set actor %u talk name '", act);
        {
            char c;
            char *n = a->name;
            do
            {
                c = fetchScriptByte();
                *n++ = c;
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
		stack[curScript].resultVarNumber++;
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
		verb_kill(slot);
        DEBUG_PRINTF("VerbKill %d\n", slot);
		break;

	case 0xFF:	// Verb On/Off
		verb = fetchScriptByte();
		state = fetchScriptByte();
		slot = verb_getSlot(verb, 0);
		verbs[slot].curmode = state;
        DEBUG_PRINTF("VerbOnOff %d\n", slot);
		break;

	default: {	// New Verb
		int x = fetchScriptByte();// * 8;
		int y = fetchScriptByte();// * 8;
		slot = getVarOrDirectByte(PARAM_1) + 1;
		int prep = fetchScriptByte(); // Only used in V1?
        x += 1;//8;

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
        DEBUG_ASSERT(slot < _numVerbs, "op_verbOps");
        VerbSlot *vs = &verbs[slot];
        vs->verbid = verb;
        vs->x = x;
        vs->y = y;

        // static const char keyboard[] = {
        //         'q','w','e','r',
        //         'a','s','d','f',
        //         'z','x','c','v'
        //     };
        // if (1 <= slot && slot <= ARRAYSIZE(keyboard))
        //     vs->key = keyboard[slot - 1];
		// }

        // get the name
        char *s = vs->name;
        while (*s++ = fetchScriptByte())
            ;
        DEBUG_ASSERT(s - vs->name < VERB_NAME_SIZE, "op_verbOps name");
        DEBUG_PRINTF("New verb '%s' at %d %d\n", vs->name, vs->x, vs->y);
        }
		break;
	}

	// Force redraw of the modified verb slot
	graphics_drawVerb(&verbs[slot]);
	// verbMouseOver(0);
}

static void op_isSoundRunning(void)
{
	uint8_t snd;
	getResultPos();
	snd = getVarOrDirectByte(PARAM_1);
	//if (snd)
	//	snd = _sound->isSoundRunning(snd);
    snd = 0;
	setResult(snd);
}

static void op_breakHere(void)
{
    //DEBUG_PUTS("breakHere\n");
    //stopScript();
    exitFlag = 1;
}

static void op_delayVariable(void)
{
	stack[curScript].delay = getVar();
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

static void op_isNotEqualZero(void)
{
	uint16_t a = getVar();
	jumpRelative(a != 0);
}

static void op_delay(void)
{
    uint8_t a = fetchScriptByte() ^ 0xff;
    uint8_t b = fetchScriptByte() ^ 0xff;
    uint8_t c = fetchScriptByte() ^ 0xff;
    //DEBUG_PRINTF("delay %x %x %x\n", a, b, c);
    stack[curScript].delay = a | (b << 8) | ((uint32_t)c << 16);
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
    DEBUG_PRINTF("Start script with id %u\n", script);
    pushScript(script, scripts[script].room, scripts[script].roomoffs, 4);
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
	scummVars[stack[curScript].resultVarNumber] += a;
}

static void setUserState(uint8_t state)
{
    // if (state & USERSTATE_SET_IFACE) {			// Userface
    //     _userState = (_userState & ~USERSTATE_IFACE_ALL) | (state & USERSTATE_IFACE_ALL);
    // }

    // if (state & USERSTATE_SET_FREEZE) {		// Freeze
    //     if (state & USERSTATE_FREEZE_ON)
    //         freezeScripts(0);
    //     else
    //         unfreezeScripts();
    // }

    // Cursor Show/Hide
    if (state & USERSTATE_SET_CURSOR)
    {
        //_userState = (_userState & ~USERSTATE_CURSOR_ON) | (state & USERSTATE_CURSOR_ON);
        if (state & USERSTATE_CURSOR_ON)
        {
            //_userPut = 1;
            cursor_setState(1);
        }
        else
        {
            //_userPut = 0;
            cursor_setState(0);
        }
    }

    // // Hide all verbs and inventory
    // Common::Rect rect;
    // rect.top = _virtscr[kVerbVirtScreen].topline;
    // rect.bottom = _virtscr[kVerbVirtScreen].topline + 8 * 88;
    // rect.right = _virtscr[kVerbVirtScreen].w - 1;
    //     rect.left = 16;
    // restoreBackground(rect);

    // // Draw all verbs and inventory
    // redrawVerbs();
    // runInventoryScript(1);
}

static void op_cursorCommand(void)
{
	uint16_t cmd = getVarOrDirectWord(PARAM_1);
	uint8_t state = cmd >> 8;

    DEBUG_PRINTF("Set cursor state %x user state %x\n", cmd & 0xff, state);
	if (cmd & 0xFF) {
		scummVars[VAR_CURSORSTATE] = cmd & 0xFF;
	}

	setUserState(state);
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

static void op_endCutscene(void)
{
    scummVars[VAR_OVERRIDE] = 0;

	// vm.cutSceneStackPointer = 0;
	// vm.cutSceneScript[0] = 0;
	// vm.cutScenePtr[0] = 0;
	// VAR(VAR_CURSORSTATE) = vm.cutSceneData[1];
	// // Reset user state to values before cutscene
	// setUserState(vm.cutSceneData[0] | USERSTATE_SET_IFACE | USERSTATE_SET_CURSOR | USERSTATE_SET_FREEZE);

    //actorFollowCamera(VAR(VAR_EGO));
}

static void op_findObject(void)
{
    Object *obj = 0;
	getResultPos();
    // objects store x y without multipliers
	uint16_t x = getVarOrDirectByte(PARAM_1);// * V12_X_MULTIPLIER;
	uint16_t y = getVarOrDirectByte(PARAM_2);// * V12_Y_MULTIPLIER;
	obj = object_find(x, y);
	// if (obj == 0 && (_game.platform == Common::kPlatformNES) && (_userState & USERSTATE_IFACE_INVENTORY)) {
	// 	if (_mouseOverBoxV2 >= 0 && _mouseOverBoxV2 < 4)
	// 		obj = findInventory(VAR(VAR_EGO), _mouseOverBoxV2 + _inventoryOffset + 1);
	// }
    if (obj)
    {
        DEBUG_PRINTF("Found object %d x=%d y=%d\n", obj->obj_nr, obj->x, obj->y);
	    setResult(obj->obj_nr);
    }
    else
    {
        setResult(0);
    }
}

static void op_drawSentence(void)
{
	// Common::Rect sentenceline;
	// const byte *temp;
    uint8_t slot = verb_getSlot(scummVars[VAR_SENTENCE_VERB], 0);
    char buf[61];
    char *s = buf;

	// if (!((_userState & USERSTATE_IFACE_SENTENCE) ||
	//       (_game.platform == Common::kPlatformNES && (_userState & USERSTATE_IFACE_ALL))))
	// 	return;

	// if (getResourceAddress(rtVerb, slot))
	// 	_sentenceBuf = (char *)getResourceAddress(rtVerb, slot);
	// else
	// 	return;

    if (scummVars[VAR_SENTENCE_OBJECT1] > 0) {
        uint8_t len = getObjOrActorName(s + 1, scummVars[VAR_SENTENCE_OBJECT1]);
        if (len)
        {
            *s++ = ' ';
            s += len;
        }
    }

	// if (0 < VAR(VAR_SENTENCE_PREPOSITION) && VAR(VAR_SENTENCE_PREPOSITION) <= 4) {
	// 	drawPreposition(VAR(VAR_SENTENCE_PREPOSITION));
	// }

    if (scummVars[VAR_SENTENCE_OBJECT2] > 0) {
        uint8_t len = getObjOrActorName(s + 1, scummVars[VAR_SENTENCE_OBJECT1]);
        if (len)
        {
            *s++ = ' ';
            s += len;
        }
    }

    *s = 0;

	// _string[2].charset = 1;
	// _string[2].ypos = _virtscr[kVerbVirtScreen].topline;
	// _string[2].xpos = 0;
	// _string[2].right = _virtscr[kVerbVirtScreen].w - 1;
	// if (_game.platform == Common::kPlatformNES) {
	// 	_string[2].xpos = 16;
	// 	_string[2].color = 0;

	// byte string[80];
	// const char *ptr = _sentenceBuf.c_str();
	// int i = 0, len = 0;

	// Maximum length of printable characters
	// int maxChars = (_game.platform == Common::kPlatformNES) ? 60 : 40;
	// while (*ptr) {
	// 	if (*ptr != '@')
	// 		len++;
	// 	if (len > maxChars) {
	// 		break;
	// 	}

	// 	string[i++] = *ptr++;

	// 	if (_game.platform == Common::kPlatformNES && len == 30) {
	// 		string[i++] = 0xFF;
	// 		string[i++] = 8;
	// 	}
	// }
	// string[i] = 0;

    // sentenceline.top = _virtscr[kVerbVirtScreen].topline;
    // sentenceline.bottom = _virtscr[kVerbVirtScreen].topline + 16;
    // sentenceline.left = 16;
    // sentenceline.right = _virtscr[kVerbVirtScreen].w - 1;

	// restoreBackground(sentenceline);

    // TODO: special codes
    DEBUG_PRINTF("Draw sentence '%s'\n", buf);
    graphics_print(buf);
}

static void op_doSentence(void)
{
    uint8_t verb;

    verb = getVarOrDirectByte(PARAM_1);
    if (verb == 0xFC) {
        // _sentenceNum = 0;
        // stopScript(SENTENCE_SCRIPT);
        return;
    }
    if (verb == 0xFB) {
        // resetSentence();
        return;
    }

	//st = &_sentence[_sentenceNum++];

    uint16_t objectA = getVarOrDirectWord(PARAM_2);
    uint16_t objectB = getVarOrDirectWord(PARAM_3);
	uint8_t opcode = fetchScriptByte();
    DEBUG_PRINTF("doSentence %d %d %d %d\n", verb, objectA, objectB, opcode);
	switch (opcode) {
	case 0:
		// Do nothing (besides setting up the sentence above)
		break;
	case 1:
		// Execute the sentence
		//_sentenceNum--;

		if (verb == 254) {
			//ScummEngine::stopObjectScript(st->objectA);
		} else {
			// bool isBackgroundScript;
			// bool isSpecialVerb;
			// if (verb != 253 && verb != 250) {
			// 	scummVars[VAR_ACTIVE_VERB] = verb;
			// 	scummVars[VAR_ACTIVE_OBJECT1] = objectA;
			// 	scummVars[VAR_ACTIVE_OBJECT2] = objectB;

			// 	isBackgroundScript = false;
			// 	isSpecialVerb = false;
			// } else {
			// 	isBackgroundScript = (st->verb == 250);
			// 	isSpecialVerb = true;
			// 	verb = 253;
			// }

			// Check if an object script for this object is already running. If
			// so, reuse its script slot. Note that we abuse two script flags:
			// freezeResistant and recursive. We use them to track two
			// script flags used in V1/V2 games. The main reason we do it this
			// ugly evil way is to avoid having to introduce yet another save
			// game revision.
			// int slot = -1;
			// ScriptSlot *ss;
			// int i;

			// ss = vm.slot;
			// for (i = 0; i < NUM_SCRIPT_SLOT; i++, ss++) {
			// 	if (st->objectA == ss->number &&
			// 		ss->freezeResistant == isBackgroundScript &&
			// 		ss->recursive == isSpecialVerb &&
			// 		(ss->where == WIO_ROOM || ss->where == WIO_INVENTORY || ss->where == WIO_FLOBJECT)) {
			// 		slot = i;
			// 		break;
			// 	}
			// }

			// runObjectScript(st->objectA, st->verb, isBackgroundScript, isSpecialVerb, NULL, slot);
		}
		break;
	case 2:
		// Print the sentence
		//_sentenceNum--;

		scummVars[VAR_SENTENCE_VERB] = verb;
		scummVars[VAR_SENTENCE_OBJECT1] = objectA;
		scummVars[VAR_SENTENCE_OBJECT2] = objectB;

        op_drawSentence();
		break;
	default:
		DEBUG_PUTS("doSentence error\n");
        DEBUG_HALT;
	}
}

static void op_ifState01(void)
{
    ifStateCommon(kObjectStatePickupable);
}

void executeScript(void)
{
    if (stack[curScript].delay)
    {
        //DEBUG_PRINTF("Delay %u\n", (uint16_t)delay);
        // if (delay > 100)
        //     delay -= 100;
        // else
        //     delay = 0;
        --stack[curScript].delay;
        return;
    }
    while (!exitFlag)
    {
        //DEBUG_PRINTF("EXEC %u 0x%x\n", stack[curScript].id, stack[curScript].offset - 4);
        opcode = fetchScriptByte();
        //DEBUG_PRINTF("--- opcode 0x%x\n", opcode);
        switch (opcode)
        {
        case 0x00:
            op_stopObjectCode();
            break;
        case 0x01:
            op_putActor();
            break;
        case 0x02:
            op_startMusic();
            break;
        case 0x03:
            op_getActorRoom();
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
        case 0x47:
            op_clearState08();
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
            op_stopScript();
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
        case 0x7c:
            op_isSoundRunning();
            break;
        case 0x80:
            op_breakHere();
            break;
        case 0x88:
            op_isNotEqual();
            break;
        case 0x9a:
            op_move();
            break;
        case 0xa0:
            op_stopObjectCode();
            break;
        case 0xa8:
            op_isNotEqualZero();
            break;
        case 0xab:
            op_switchCostumeSet();
            break;
        case 0xc0:
            op_endCutscene();
            break;
        case 0xc8:
            op_isEqual();
            break;
        case 0xd9:
            op_doSentence();
            break;
        case 0xf5:
            op_findObject();
            break;
        case 0xf9:
            op_doSentence();
            break;
        case 0xff:
            op_ifState01();
            break;
        default:
            DEBUG_PRINTF("Unknown opcode %x\n", opcode);
            DEBUG_HALT;
            break;
        }
    }
}

void runScript(uint8_t s)
{
    DEBUG_PRINTF("New script id %u\n", s);
    pushScript(s, scripts[s].room, scripts[s].roomoffs, 4);
    //executeScript();
}

void processScript(void)
{
    for (curScript = 0 ; curScript < MAX_SCRIPTS ; ++curScript)
    {
        if (stack[curScript].id)
        {
            exitFlag = 0;
            switchScriptPage(curScript);
            executeScript();
        }
    }
}

void runRoomScript(uint8_t id, uint8_t room, uint16_t offs)
{
    pushScript(id, room, offs, 0);
}

enum {
	MBS_LEFT_CLICK = 0x8000,
	MBS_RIGHT_CLICK = 0x4000,
	MBS_MOUSE_MASK = (MBS_LEFT_CLICK | MBS_RIGHT_CLICK),
	MBS_MAX_KEY	= 0x0200
};

__sfr __banked __at 0x7ffe IO_7FFE;

void updateScummVars(void)
{
    // VAR(VAR_CAMERA_POS_X) = (camera._cur.x >> V12_X_SHIFT);

    // We use shifts below instead of dividing by V12_X_MULTIPLIER resp.
    // V12_Y_MULTIPLIER to handle negative coordinates correctly.
    // This fixes e.g. bugs #1328131 and #1537595.
    scummVars[VAR_VIRT_MOUSE_X] = cursorX >> V12_X_SHIFT;
    scummVars[VAR_VIRT_MOUSE_Y] = cursorY >> V12_Y_SHIFT;

    // Adjust mouse coordinates as narrow rooms in NES are centered
    // if (_game.platform == Common::kPlatformNES && _NESStartStrip > 0) {
    //     VAR(VAR_VIRT_MOUSE_X) -= 2;
    //     if (VAR(VAR_VIRT_MOUSE_X) < 0)
    //         VAR(VAR_VIRT_MOUSE_X) = 0;
    // }

	// // 'B' is used to skip cutscenes in the NES version of Maniac Mansion
	// } else if (_game.id == GID_MANIAC &&_game.platform == Common::kPlatformNES) {
	// 	if (lastKeyHit.keycode == Common::KEYCODE_b && lastKeyHit.hasFlags(Common::KBD_SHIFT))
	// 		lastKeyHit = Common::KeyState(Common::KEYCODE_ESCAPE);

	// // On Alt-F5 prepare savegame for the original save/load dialog.
	// if (lastKeyHit.keycode == Common::KEYCODE_F5 && lastKeyHit.hasFlags(Common::KBD_ALT)) {
	// 	prepareSavegame();
	// 	if (_game.id == GID_MANIAC && _game.version == 0) {
	// 		runScript(2, 0, 0, 0);
	// 	}
	// 	if (_game.id == GID_MANIAC &&_game.platform == Common::kPlatformNES) {
	// 		runScript(163, 0, 0, 0);
	// 	}
	// }

    // mouse and keyboard input
    uint16_t s = 0;
    if (!(IO_7FFE & 1))
        s = MBS_LEFT_CLICK; 
	scummVars[VAR_KEYPRESS] = s;

	// if (VAR_KEYPRESS != 0xFF && _mouseAndKeyboardStat) {		// Key Input
	// 	if (315 <= _mouseAndKeyboardStat && _mouseAndKeyboardStat <= 323) {
	// 		// Convert F-Keys for V1/V2 games (they start at 1)
	// 		VAR(VAR_KEYPRESS) = _mouseAndKeyboardStat - 314;
	// 	} else {
	// 		VAR(VAR_KEYPRESS) = _mouseAndKeyboardStat;
	// 	}
	// }
}
