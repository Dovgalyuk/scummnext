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
#include "engine.h"
#include "box.h"
#include "string.h"

#define MAX_SCRIPTS 12

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

int16_t scummVars[_numVariables];
static uint8_t curScript;
static uint8_t opcode;
static uint8_t exitFlag;

// mapped in MMU1 to pages 32-...
// can be switched for different scripts
extern uint8_t scriptBytes[4096];
extern uint16_t script_id;
extern uint8_t script_room;
extern uint16_t script_roomoffs;
extern uint16_t script_offset;
extern uint32_t script_delay;
extern uint16_t script_resultVarNumber;

#define FIRST_BANK 32

typedef struct SentenceTab {
	uint8_t verb;
	uint8_t preposition;
	uint16_t objectA;
	uint16_t objectB;
	uint8_t freezeCount;
} SentenceTab;

#define SENTENCE_SCRIPT 2
#define MAX_SENTENCE 4
static SentenceTab sentence[MAX_SENTENCE];
static uint8_t sentenceNum;

/////////////////////////////////////////////////////////////////////////
// scripting internals
/////////////////////////////////////////////////////////////////////////

static void switchScriptPage(uint8_t s)
{
    //DEBUG_PRINTF("Select page %u\n", FIRST_BANK + s);
    ZXN_WRITE_MMU1(FIRST_BANK + s);
}

static void loadScriptBytes(void)
{
    HROOM r = openRoom(script_room);
    esx_f_seek(r, script_roomoffs, ESX_SEEK_SET);
    //readResource(r, scriptBytes, sizeof(scriptBytes));
    // always read up to 4k, because object scripts do not include size field
    readBuffer(r, scriptBytes, sizeof(scriptBytes));
    closeRoom(r);
    // for (int8_t i = 0 ; i < 16 ; ++i)
    //     DEBUG_PRINTF("%x ", scriptBytes[i]);
    // DEBUG_PUTS("\n");
}

static uint8_t findEmptyFrame(void)
{
    uint8_t f;
    for (f = 0 ; f < MAX_SCRIPTS ; ++f)
    {
        switchScriptPage(f);
        if (script_id == 0)
        {
            switchScriptPage(curScript);
            return f;
        }
    }
    DEBUG_ASSERT(0, "Number of scripts exceeded\n");
}

static uint8_t fetchScriptByte(void)
{
    return scriptBytes[script_offset++];
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
	writeVar(script_resultVarNumber, value);
}

static void stopScriptIdx(uint8_t index)
{
    switchScriptPage(index);
    DEBUG_PRINTF("Stopping the script %u room %u/%u offset %x\n",
        script_id, script_room, script_roomoffs,
        script_offset - 4);
    script_id = 0;
    if (index == curScript)
        exitFlag = 1;
    switchScriptPage(curScript);
}

static uint8_t getScriptIndex(uint8_t id)
{
    if (id == 0)
        return MAX_SCRIPTS;
    uint8_t i;
    for (i = 0 ; i < MAX_SCRIPTS ; ++i)
    {
        switchScriptPage(i);
        if (script_id == id)
            break;
    }
    switchScriptPage(curScript);
    return i;
}

static void stopScript(uint8_t id)
{
    uint8_t idx = getScriptIndex(id);
    if (idx == MAX_SCRIPTS)
        return;
    stopScriptIdx(idx);
}

static void op_stopObjectCode(void)
{
    stopScriptIdx(curScript);
}

static void op_stopScript(void)
{
    uint8_t s = getVarOrDirectByte(PARAM_1);

    DEBUG_PRINTF("Stopping from script %u room %u/%u next offset %x\n",
        script_id, script_room, script_roomoffs,
        script_offset);

    // WORKAROUND bug #4112: If you enter the lab while Dr. Fred has the powered turned off
    // to repair the Zom-B-Matic, the script will be stopped and the power will never turn
    // back on. This fix forces the power on, when the player enters the lab,
    // if the script which turned it off is running
    // if (_game.id == GID_MANIAC && _roomResource == 4 && isScriptRunning(MM_SCRIPT(138))) {
        // TODO

    uint8_t i = curScript;
    if (s != 0)
    {
        i = getScriptIndex(s);
        if (i == MAX_SCRIPTS)
        {
            DEBUG_PRINTF("Stopping the script %u which is not executing\n", s);
            return;
        }
    }

    stopScriptIdx(i);
}

static void getResultPos(void)
{
    script_resultVarNumber = fetchScriptByte();
}

static void pushScript(uint16_t id, uint8_t room, uint16_t roomoffs, uint16_t offset)
{
    uint8_t s = findEmptyFrame();
    DEBUG_PRINTF("Starting script %u room %u offset %u in frame %u\n", id, room, roomoffs, s);
    switchScriptPage(s);
    script_id = id;
    script_room = room;
    script_roomoffs = roomoffs;
    script_offset = offset;
    loadScriptBytes();
    switchScriptPage(curScript);
}

static void parseString(uint8_t actor)
{
    uint8_t *message = message_new();
    uint8_t *ptr = message;
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

	actor_talk(actor, message);
}

static void jumpRelative(int cond)
{
	int16_t offset = fetchScriptWord();
	if (!cond)
		script_offset += offset;
//    DEBUG_PRINTF("   jump to %x\n", stack[curScript].offset + offset);
}

static uint8_t isScriptRunning(uint8_t s)
{
    uint8_t i = getScriptIndex(s);
    return i != MAX_SCRIPTS;
}

static uint16_t getActiveObject(void)
{
	return getVarOrDirectWord(PARAM_1);
}

static void ifStateCommon(uint16_t type)
{
	uint8_t obj = getActiveObject();

	jumpRelative((object_getState(obj) & type) != 0);
}

///////////////////////////////////////////////////////////
// opcodes
///////////////////////////////////////////////////////////

static void op_breakHere(void)
{
    //DEBUG_PUTS("breakHere\n");
    //stopScript();
    exitFlag = 1;
}

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
	uint8_t xpos, ypos;
    uint16_t idx;

	idx = getVarOrDirectWord(PARAM_1);
	xpos = getVarOrDirectByte(PARAM_2);
	ypos = getVarOrDirectByte(PARAM_3);
    DEBUG_PRINTF("Draw object %u at %u,%u\n", idx, xpos, ypos);

    Object *obj = object_get(idx);
    //DEBUG_ASSERT(obj, "op_drawObject");

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

    object_setState(idx, object_getState(idx) | kObjectState_08);
	// putState(obj, getState(od->obj_nr) | kObjectState_08);
}

static void op_setState08(void)
{
 	uint16_t j = getVarOrDirectWord(PARAM_1);
    //Object *obj = object_get(j);
    //DEBUG_ASSERT(obj, "op_setState08");
    DEBUG_PRINTF("setState08 %u\n", j);

    object_setState(j, object_getState(j) | kObjectState_08);
	// markObjectRectAsDirty(obj);
	// clearDrawObjectQueue();

	graphics_drawObject(object_get(j));
}

static void op_clearState08(void)
{
    uint16_t j = getActiveObject();
    //Object *obj = object_get(j);
    DEBUG_PRINTF("clearState08 %u\n", j);

    object_setState(j, object_getState(j) & ~kObjectState_08);
    //markObjectRectAsDirty(obj);
    //clearDrawObjectQueue();

	graphics_drawObject(object_get(j));
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

static void op_walkActorToActor(void)
{
	uint8_t nr = getVarOrDirectByte(PARAM_1);
	uint8_t nr2 = getVarOrDirectByte(PARAM_2);
	uint8_t dist = fetchScriptByte();

	if (!actor_isInCurrentRoom(nr))
		return;

	if (!actor_isInCurrentRoom(nr2))
		return;

	actor_walkToActor(nr, nr2, dist);
}

static void op_getObjectOwner(void)
{
	getResultPos();
	setResult(object_getOwner(getVarOrDirectWord(PARAM_1)));
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

static void op_stopMusic(void)
{
    // nothig but stop all sounds
}

static void op_putActorInRoom(void)
{
	uint8_t act = getVarOrDirectByte(PARAM_1);
    uint8_t room = getVarOrDirectByte(PARAM_2);
    actor_setRoom(act, room);
}

static void op_waitForMessage(void)
{
	if (scummVars[VAR_HAVE_MSG])
    {
		script_offset--;
		op_breakHere();
	}
}

static void op_actorOps(void)
{
	uint8_t act = getVarOrDirectByte(PARAM_1);
	uint8_t arg = getVarOrDirectByte(PARAM_2);

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

static void op_putActorAtObject(void)
{
    uint8_t x, y;
    uint8_t a = getVarOrDirectByte(PARAM_1);
	uint8_t obj = getVarOrDirectWord(PARAM_2);
	if (object_whereIs(obj) != WIO_NOT_FOUND)
    {
		object_getXY(obj, &x, &y);
        boxes_adjustXY(&x, &y);
	} else {
		x = 30 / 8;
		y = 60 / 8;
	}

    actor_put(a, x, y);
}

static void op_printEgo(void)
{
    uint8_t act = scummVars[VAR_EGO];
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
		script_resultVarNumber++;
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
		uint8_t x = fetchScriptByte();// * 8;
		uint8_t y = fetchScriptByte();// * 8;
		slot = getVarOrDirectByte(PARAM_1) + 1;
		uint8_t prep = fetchScriptByte(); // Only used in V1?
        x += 1;//8;

        // vs->color = 1;
        // vs->hicolor = 1;
        // vs->dimcolor = 1;
		// vs->type = kTextVerbType;
		// vs->charset_nr = _string[0]._default.charset;
		// vs->saveid = 0;
		// vs->key = 0;
		// vs->center = 0;
		// vs->imgindex = 0;
		// vs->prep = prep;

		// vs->curRect.left = x;
		// vs->curRect.top = y;
        DEBUG_ASSERT(slot < _numVerbs, "op_verbOps");
        verb_kill(slot);
        VerbSlot *vs = &verbs[slot];
        vs->verbid = verb;
        vs->x = x;
        vs->y = y;
		vs->curmode = 1;

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
        // DEBUG_PRINTF("New verb '%s' at %d %d\n", vs->name, vs->x, vs->y);
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

void op_equalZero()
{
	uint16_t a = getVar();
	jumpRelative(a == 0);
}

static void op_delayVariable(void)
{
	script_delay = getVar();
	//vm.slot[_currentScript].status = ssPaused;
	op_breakHere();
}

static void op_assignVarByte(void)
{
	getResultPos();
	setResult(fetchScriptByte());
}

static void op_isGreaterEqual(void)
{
	uint16_t a = getVar();
	uint16_t b = getVarOrDirectWord(PARAM_1);
	jumpRelative(b >= a);
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
    script_delay = a | (b << 8) | ((uint32_t)c << 16);
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
		script_offset -= 2;
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

static void op_isLess(void)
{
	uint16_t a = getVar();
	uint16_t b = getVarOrDirectWord(PARAM_1);

	jumpRelative(b < a);
}

static void op_increment(void)
{
	getResultPos();
	setResult(scummVars[script_resultVarNumber] + 1);
}

static void op_isEqual(void)
{
    uint8_t var = fetchScriptByte();
	uint16_t a = readVar(var);
	uint16_t b = getVarOrDirectWord(PARAM_1);
	jumpRelative(b == a);
}

static void op_pickupObject(void)
{
	int id = getVarOrDirectWord(PARAM_1);

    Object *obj = object_get(id);
	if (!obj)
		return;

	if (object_whereIs(id) == WIO_INVENTORY)	/* Don't take an */
		return;											/* object twice */

    DEBUG_PRINTF("Pickup object %u by %u\n", id, scummVars[VAR_EGO]);

	//addObjectToInventory(obj, _roomResource);
	//markObjectRectAsDirty(obj);
    object_setOwner(id, scummVars[VAR_EGO]);
	object_setState(id, object_getState(id) | kObjectState_08 | kObjectStateUntouchable);
	//clearDrawObjectQueue();

	//runInventoryScript(1);
	//if (_game.platform == Common::kPlatformNES)
    //		_sound->addSoundToQueue(51);	// play 'pickup' sound
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
	scummVars[script_resultVarNumber] += a;
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
    runInventoryScript(1);
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
    uint8_t ego = scummVars[VAR_EGO];

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
    actor_put(ego, 0, 0);
    actor_setRoom(ego, room);

	int8_t x = fetchScriptByte();
	int8_t y = fetchScriptByte();

	startScene(room);

    uint8_t x2, y2;
    object_getXY(obj, &x2, &y2);
    boxes_adjustXY(&x2, &y2);
    actor_put(ego, x2, y2);
	// a->setDirection(dir + 180);

	// camera._dest.x = camera._cur.x = a->getPos().x;
	// setCameraAt(a->getPos().x, a->getPos().y);
    camera_setX(actors[ego].x);
    camera_followActor(ego);

	// _fullRedraw = true;

	// resetSentence();

	if (x >= 0 && y >= 0) {
        actor_startWalk(ego, x, y);
    }
    // script is started in startScene
	//runScript(5);
}

static void op_isScriptRunning(void)
{
//    DEBUG_PUTS("isScriptRunning\n");
	getResultPos();
	setResult(isScriptRunning(getVarOrDirectByte(PARAM_1)));
}

static void op_setOwnerOf(void)
{
	uint16_t obj;
    uint8_t owner;

	obj = getVarOrDirectWord(PARAM_1);
	owner = getVarOrDirectByte(PARAM_2);

	object_setOwner(obj, owner);
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

static void op_getBitVar(void)
{
	getResultPos();
	int var = fetchScriptWord();
	uint8_t a = getVarOrDirectByte(PARAM_1);

	int bit_var = var + a;
	int bit_offset = bit_var & 0x0f;
	bit_var >>= 4;

	setResult((scummVars[bit_var] & (1 << bit_offset)) ? 1 : 0);
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
        //DEBUG_PRINTF("Found object %d x=%d y=%d\n", obj->obj_nr, obj->x, obj->y);
	    setResult(obj->obj_nr);
    }
    else
    {
        setResult(0);
    }
}

static void op_walkActorToObject(void)
{
	uint8_t actor = getVarOrDirectByte(PARAM_1);
	uint16_t obj = getVarOrDirectWord(PARAM_2);
    DEBUG_PRINTF("Actor %u walk to object %u\n", actor, obj);
	if (object_whereIs(obj) != WIO_NOT_FOUND) {
		actor_walkToObject(actor, obj);
	}
}

static void op_drawSentence(void)
{
	// Common::Rect sentenceline;
	// const byte *temp;
    uint8_t slot = verb_getSlot(scummVars[VAR_SENTENCE_VERB], 0);
    char *buf = message_new();
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
    message_print(buf);
}

static void op_doSentence(void)
{
    uint8_t verb;

    verb = getVarOrDirectByte(PARAM_1);
    if (verb == 0xFC) {
        sentenceNum = 0;
        stopScript(SENTENCE_SCRIPT);
        return;
    }
    if (verb == 0xFB) {
        // resetSentence();
        return;
    }

	SentenceTab *st = &sentence[sentenceNum++];

	st->verb = verb;
	st->objectA = getVarOrDirectWord(PARAM_2);
	st->objectB = getVarOrDirectWord(PARAM_3);
	st->preposition = (st->objectB != 0);
	st->freezeCount = 0;

	uint8_t opcode = fetchScriptByte();
    DEBUG_PRINTF("doSentence %d: %d %d %d\n", opcode, verb, st->objectA, st->objectB);
	switch (opcode) {
	case 0:
		// Do nothing (besides setting up the sentence above)
		break;
	case 1:
		// Execute the sentence
		sentenceNum--;

		if (verb == 254) {
			//ScummEngine::stopObjectScript(st->objectA);
		} else {
			// bool isBackgroundScript;
			// bool isSpecialVerb;
			if (verb != 253 && verb != 250) {
				scummVars[VAR_ACTIVE_VERB] = verb;
				scummVars[VAR_ACTIVE_OBJECT1] = st->objectA;
				scummVars[VAR_ACTIVE_OBJECT2] = st->objectB;

			// 	isBackgroundScript = false;
			// 	isSpecialVerb = false;
			} else {
			// 	isBackgroundScript = (st->verb == 250);
			// 	isSpecialVerb = true;
				verb = 253;
			}

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
            runObjectScript(st->objectA, st->verb);
		}
		break;
	case 2:
		// Print the sentence
		sentenceNum--;

		scummVars[VAR_SENTENCE_VERB] = verb;
		scummVars[VAR_SENTENCE_OBJECT1] = st->objectA;
		scummVars[VAR_SENTENCE_OBJECT2] = st->objectB;

        op_drawSentence();
		break;
	default:
		DEBUG_PUTS("doSentence error\n");
        DEBUG_HALT;
	}
}

static void op_getDist(void)
{
	getResultPos();

	uint16_t o1 = getVarOrDirectWord(PARAM_1);
	uint16_t o2 = getVarOrDirectWord(PARAM_2);

    uint16_t r = getObjActToObjActDist(o1, o2);

	setResult(r);
}

static void op_ifState01(void)
{
    ifStateCommon(kObjectStatePickupable);
}

static void op_ifState08(void)
{
    ifStateCommon(kObjectState_08);
}

typedef void (*OpcodeFunc)(void);

static OpcodeFunc opcodes[0x100] = {
    [0x00] = op_stopObjectCode,
    [0x01] = op_putActor,
    [0x02] = op_startMusic,
    [0x03] = op_getActorRoom,
    [0x05] = op_drawObject,
    [0x07] = op_setState08,
    [0x08] = op_isNotEqual,
    [0x0c] = op_resourceRoutines,
    [0x0d] = op_walkActorToActor,
    [0x0e] = op_putActorAtObject,
    [0x10] = op_getObjectOwner,
    [0x11] = op_animateActor,
    [0x12] = op_panCameraTo,
    [0x13] = op_actorOps,
    [0x14] = op_print,
    [0x16] = op_getRandomNr,
    [0x18] = op_jumpRelative,
    [0x19] = op_doSentence,
    [0x1a] = op_move,
    [0x1b] = op_setBitVar,
    [0x1c] = op_startSound,
    [0x1e] = op_walkActorTo,
    [0x20] = op_stopMusic,
    [0x24] = op_loadRoomWithEgo,
    [0x26] = op_setVarRange,
    [0x28] = op_equalZero,
    [0x2b] = op_delayVariable,
    [0x2c] = op_assignVarByte,
    [0x2d] = op_putActorInRoom,
    [0x2e] = op_delay,
    [0x32] = op_setCameraAt,
    [0x38] = op_isLessEqual,
    [0x3b] = op_waitForActor,
    [0x3c] = op_stopSound,
    [0x40] = op_cutscene,
    [0x42] = op_startScript,
    [0x43] = op_getActorX,
    [0x44] = op_isLess,
    [0x46] = op_increment,
    [0x47] = op_clearState08,
    [0x48] = op_isEqual,
    [0x4f] = op_ifState08,
    [0x50] = op_pickupObject,
    [0x52] = op_actorFollowCamera,
    [0x53] = op_actorOps,
    [0x58] = op_beginOverride,
    [0x5a] = op_add,
    [0x5b] = op_setBitVar,
    [0x60] = op_cursorCommand,
    [0x62] = op_stopScript,
    [0x68] = op_isScriptRunning,
    [0x69] = op_setOwnerOf,
    [0x70] = op_lights,
    [0x72] = op_loadRoom,
    [0x78] = op_isGreater,
    [0x7a] = op_verbOps,
    [0x7c] = op_isSoundRunning,
    [0x80] = op_breakHere,
    [0x81] = op_putActor,
    [0x84] = op_isGreaterEqual,
    [0x85] = op_drawObject,
    [0x88] = op_isNotEqual,
    [0x90] = op_getObjectOwner,
    [0x91] = op_getObjectOwner,
    [0x94] = op_print,
    [0x9a] = op_move,
    [0xa0] = op_stopObjectCode,
    [0xa8] = op_isNotEqualZero,
    [0xab] = op_switchCostumeSet,
    [0xac] = op_drawSentence,
    [0xad] = op_putActorInRoom,
    [0xae] = op_waitForMessage,
    [0xb1] = op_getBitVar,
    [0xbb] = op_waitForActor,
    [0xc0] = op_endCutscene,
    [0xc8] = op_isEqual,
    [0xce] = op_putActorAtObject,
    [0xcf] = op_ifState08,
    [0xd8] = op_printEgo,
    [0xd9] = op_doSentence,
    [0xf4] = op_getDist,
    [0xf5] = op_findObject,
    [0xf6] = op_walkActorToObject,
    [0xf9] = op_doSentence,
    [0xfa] = op_verbOps,
    [0xfe] = op_walkActorTo,
    [0xff] = op_ifState01,
};

void executeScript(void)
{
    if (script_delay)
    {
        //DEBUG_PRINTF("Delay %u\n", (uint16_t)delay);
        // if (delay > 100)
        //     delay -= 100;
        // else
        //     delay = 0;
        --script_delay;
        return;
    }
    while (!exitFlag)
    {
        //DEBUG_PRINTF("EXEC %u 0x%x\n", stack[curScript].id, stack[curScript].offset - 4);
        opcode = fetchScriptByte();
        OpcodeFunc f = opcodes[opcode];
        if (!f)
        {
            DEBUG_PRINTF("Unknown opcode %x\n", opcode);
            DEBUG_HALT;
        }
        f();
        //DEBUG_PRINTF("--- opcode 0x%x\n", opcode);
        // switch (opcode)
        // {
        // case 0x00:
        //     op_stopObjectCode();
        //     break;
        // case 0x01:
        //     op_putActor();
        //     break;
        // case 0x02:
        //     op_startMusic();
        //     break;
        // case 0x03:
        //     op_getActorRoom();
        //     break;
        // case 0x05:
        //     op_drawObject();
        //     break;
        // case 0x07:
        //     op_setState08();
        //     break;
        // case 0x08:
        //     op_isNotEqual();
        //     break;
        // case 0x0c:
        //     op_resourceRoutines();
        //     break;
        // case 0x0d:
        //     op_walkActorToActor();
        //     break;
        // case 0x0e:
        //     op_putActorAtObject();
        //     break;
        // case 0x10:
        //     op_getObjectOwner();
        //     break;
        // case 0x11:
        //     op_animateActor();
        //     break;
        // case 0x12:
        //     op_panCameraTo();
        //     break;
        // case 0x13:
        // case 0x53:
        //     op_actorOps();
        //     break;
        // case 0x14:
        //     op_print();
        //     break;
        // case 0x16:
        //     op_getRandomNr();
        //     break;
        // case 0x18:
        //     op_jumpRelative();
        //     break;
        // case 0x19:
        //     op_doSentence();
        //     break;
        // case 0x1a:
        //     op_move();
        //     break;
        // case 0x1b:
        // case 0x5b:
        //     op_setBitVar();
        //     break;
        // case 0x1c:
        //     op_startSound();
        //     break;
        // case 0x1e:
        //     op_walkActorTo();
        //     break;
        // case 0x20:
        //     op_stopMusic();
        //     break;
        // case 0x24:
        //     op_loadRoomWithEgo();
        //     break;
        // case 0x26:
        //     op_setVarRange();
        //     break;
        // case 0x28:
        //     op_equalZero();
        //     break;
        // case 0x2b:
        //     op_delayVariable();
        //     break;
        // case 0x2c:
        //     op_assignVarByte();
        //     break;
        // case 0x2d:
        //     op_putActorInRoom();
        //     break;
        // case 0x2e:
        //     op_delay();
        //     break;
        // case 0x32:
        //     op_setCameraAt();
        //     break;
        // case 0x38:
        //     op_isLessEqual();
        //     break;
        // case 0x3b:
        //     op_waitForActor();
        //     break;
        // case 0x3c:
        //     op_stopSound();
        //     break;
        // case 0x40:
        //     op_cutscene();
        //     break;
        // case 0x42:
        //     op_startScript();
        //     break;
        // case 0x43:
        //     op_getActorX();
        //     break;
        // case 0x44:
        //     op_isLess();
        //     break;
        // case 0x46:
        //     op_increment();
        //     break;
        // case 0x47:
        //     op_clearState08();
        //     break;
        // case 0x48:
        //     op_isEqual();
        //     break;
        // case 0x4f:
        //     op_ifState08();
        //     break;
        // case 0x50:
        //     op_pickupObject();
        //     break;
        // case 0x52:
        //     op_actorFollowCamera();
        //     break;
        // case 0x58:
        //     op_beginOverride();
        //     break;
        // case 0x5a:
        //     op_add();
        //     break;
        // case 0x60:
        //     op_cursorCommand();
        //     break;
        // case 0x62:
        //     op_stopScript();
        //     break;
        // // case 0x64:
        // //     op_loadRoomWithEgo();
        // //     break;
        // case 0x68:
        //     op_isScriptRunning();
        //     break;
        // case 0x69:
        //     op_setOwnerOf();
        //     break;
        // case 0x70:
        //     op_lights();
        //     break;
        // case 0x72:
        //     op_loadRoom();
        //     break;
        // case 0x78:
        //     op_isGreater();
        //     break;
        // case 0x7a:
        //     op_verbOps();
        //     break;
        // case 0x7c:
        //     op_isSoundRunning();
        //     break;
        // case 0x80:
        //     op_breakHere();
        //     break;
        // case 0x81:
        //     op_putActor();
        //     break;
        // case 0x84:
        //     op_isGreaterEqual();
        //     break;
        // case 0x85:
        //     op_drawObject();
        //     break;
        // case 0x88:
        //     op_isNotEqual();
        //     break;
        // case 0x90:
        //     op_getObjectOwner();
        //     break;
        // case 0x91:
        //     op_getObjectOwner();
        //     break;
        // case 0x94:
        //     op_print();
        //     break;
        // case 0x9a:
        //     op_move();
        //     break;
        // case 0xa0:
        //     op_stopObjectCode();
        //     break;
        // case 0xa8:
        //     op_isNotEqualZero();
        //     break;
        // case 0xab:
        //     op_switchCostumeSet();
        //     break;
        // case 0xac:
        //     op_drawSentence();
        //     break;
        // case 0xad:
        //     op_putActorInRoom();
        //     break;
        // case 0xae:
        //     op_waitForMessage();
        //     break;
        // case 0xb1:
        //     op_getBitVar();
        //     break;
        // case 0xbb:
        //     op_waitForActor();
        //     break;
        // case 0xc0:
        //     op_endCutscene();
        //     break;
        // case 0xc8:
        //     op_isEqual();
        //     break;
        // case 0xce:
        //     op_putActorAtObject();
        //     break;
        // case 0xcf:
        //     op_ifState08();
        //     break;
        // case 0xd8:
        //     op_printEgo();
        //     break;
        // case 0xd9:
        //     op_doSentence();
        //     break;
        // case 0xf4:
        //     op_getDist();
        //     break;
        // case 0xf5:
        //     op_findObject();
        //     break;
        // case 0xf6:
        //     op_walkActorToObject();
        //     break;
        // case 0xf9:
        //     op_doSentence();
        //     break;
        // case 0xfa:
        //     op_verbOps();
        //     break;
        // case 0xfe:
        //     op_walkActorTo();
        //     break;
        // case 0xff:
        //     op_ifState01();
        //     break;
        // default:
        //     DEBUG_PRINTF("Unknown opcode %x\n", opcode);
        //     DEBUG_HALT;
        //     break;
        // }
    }
}

void runScript(uint16_t s)
{
    DEBUG_PRINTF("New script id %u\n", s);
    pushScript(s, scripts[s].room, scripts[s].roomoffs, 4);
    //executeScript();
}

void runObjectScript(uint16_t objId, uint8_t verb)
{
    Object *obj = object_get(objId);
    uint8_t offs = object_getVerbEntrypoint(objId, verb);
    if (offs == 0)
        return;
    DEBUG_PRINTF("New object script %u for verb %u obcd=%d verbOffs=%d\n",
        objId, verb, obj->OBCDoffset, offs);
    pushScript(objId, currentRoom, obj->OBCDoffset + offs, 0);
}

void processScript(void)
{
    for (curScript = 0 ; curScript < MAX_SCRIPTS ; ++curScript)
    {
        switchScriptPage(curScript);
        if (script_id)
        {
            //DEBUG_PRINTF("Script %d\n", stack[curScript].id);
            exitFlag = 0;
            executeScript();
        }
    }
}

void runRoomScript(uint16_t id, uint8_t room, uint16_t offs)
{
    pushScript(id, room, offs, 0);
}

enum {
	MBS_LEFT_CLICK = 0x8000,
	MBS_RIGHT_CLICK = 0x4000,
	MBS_MOUSE_MASK = (MBS_LEFT_CLICK | MBS_RIGHT_CLICK),
	MBS_MAX_KEY	= 0x0200
};

/**
 * The area in which some click (or key press) occurred and which is passed
 * to the input script.
 */
enum ClickArea {
	kVerbClickArea = 1,
	kSceneClickArea = 2,
	kInventoryClickArea = 3,
	kKeyClickArea = 4,
	kSentenceClickArea = 5
};

void runInputScript(uint8_t clickArea, uint16_t val)
{
    DEBUG_PRINTF("Run input script %d %d\n", clickArea, val);
	scummVars[VAR_CLICK_AREA] = clickArea;
	switch (clickArea)
    {
	case kVerbClickArea:		// Verb clicked
		scummVars[VAR_CLICK_VERB] = val;
		break;
	case kInventoryClickArea:		// Inventory clicked
		scummVars[VAR_CLICK_OBJECT] = val;
		break;
	default:
		break;
	}

    runScript(4);
}

__sfr __banked __at 0x7ffe IO_7FFE;

void updateScummVars(void)
{
    // updated in camera.c
    //scummVars[VAR_CAMERA_POS_X] = cameraX >> V12_X_SHIFT;

    // We use shifts below instead of dividing by V12_X_MULTIPLIER resp.
    // V12_Y_MULTIPLIER to handle negative coordinates correctly.
    // This fixes e.g. bugs #1328131 and #1537595.
    uint8_t cam = camera_getVirtScreenX();
    scummVars[VAR_VIRT_MOUSE_X] = cam + (cursorX >> V12_X_SHIFT);
    int16_t y = cursorY - SCREEN_TOP * 8;
    if (y < 0)
        y = 0;
    scummVars[VAR_VIRT_MOUSE_Y] = y >> V12_Y_SHIFT;

    // Adjust mouse coordinates as narrow rooms in NES are centered
    // if (_game.platform == Common::kPlatformNES && _NESStartStrip > 0) {
    if (roomWidth < SCREEN_WIDTH)
    {
        scummVars[VAR_VIRT_MOUSE_X] -= 2;
        if (scummVars[VAR_VIRT_MOUSE_X] < 0)
            scummVars[VAR_VIRT_MOUSE_X] = 0;
    }

	// // 'B' is used to skip cutscenes in the NES version of Maniac Mansion
	// } else if (_game.id == GID_MANIAC &&_game.platform == Common::kPlatformNES) {
	// 	if (lastKeyHit.keycode == Common::KEYCODE_b && lastKeyHit.hasFlags(Common::KBD_SHIFT))
	// 		lastKeyHit = Common::KeyState(Common::KEYCODE_ESCAPE);

	// // On Alt-F5 prepare savegame for the original save/load dialog.
	// if (lastKeyHit.keycode == Common::KEYCODE_F5 && lastKeyHit.hasFlags(Common::KBD_ALT)) {
	// 	prepareSavegame();
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

void checkExecVerbs(void)
{
    uint16_t state = scummVars[VAR_KEYPRESS];
	//if (_userPut <= 0 || _mouseAndKeyboardStat == 0)
    if (!state)
		return;

	if (state < MBS_MAX_KEY) {
		/* Check keypresses */
		// vs = &_verbs[1];
		// for (i = 1; i < _numVerbs; i++, vs++) {
		// 	if (vs->verbid && vs->saveid == 0 && vs->curmode == 1) {
		// 		if (_mouseAndKeyboardStat == vs->key) {
		// 			// Trigger verb as if the user clicked it
		// 			runInputScript(kVerbClickArea, vs->verbid, 1);
		// 			return;
		// 		}
		// 	}
		// }

		// Simulate inventory picking and scrolling keys
		// int object = -1;

		// switch (_mouseAndKeyboardStat) {
		// case 'u': // arrow up
		// 	if (_inventoryOffset >= 2) {
		// 		_inventoryOffset -= 2;
		// 		redrawV2Inventory();
		// 	}
		// 	return;
		// case 'j': // arrow down
		// 	if (_inventoryOffset + 4 < getInventoryCount(_scummVars[VAR_EGO])) {
		// 		_inventoryOffset += 2;
		// 		redrawV2Inventory();
		// 	}
		// 	return;
		// case 'i': // object
		// 	object = 0;
		// 	break;
		// case 'o':
		// 	object = 1;
		// 	break;
		// case 'k':
		// 	object = 2;
		// 	break;
		// case 'l':
		// 	object = 3;
		// 	break;
		// default:
		// 	break;
		// }

		// if (object != -1) {
		// 	object = findInventory(_scummVars[VAR_EGO], object + 1 + _inventoryOffset);
		// 	if (object > 0)
		// 		runInputScript(kInventoryClickArea, object, 0);
		// 	return;
		// }

		// Generic keyboard input
		//runInputScript(kKeyClickArea, _mouseAndKeyboardStat, 1);
	} else if (state & MBS_MOUSE_MASK) {
        DEBUG_PRINTF("Clicked at %d %d\n", cursorX, cursorY);
        uint8_t y = cursorY / 8;
		uint8_t zone = graphics_findVirtScreen(y);
		//const uint8_t code = state & MBS_LEFT_CLICK ? 1 : 2;
		// const int inventoryArea = (_game.platform == Common::kPlatformNES) ? 48: 32;

		// This could be kUnkVirtScreen.
		// Fixes bug #1536932: "MANIACNES: Crash on click in speechtext-area"
		// if (!zone)
		// 	return;

        uint8_t x = cursorX / 8;
        DEBUG_PRINTF("Clicked at %d %d %d\n", x, y, zone);
		if (zone == kVerbVirtScreen/* && _mouse.y <= zone->topline + 8*/)
        {
		// 	// Click into V2 sentence line
		// 	runInputScript(kSentenceClickArea, 0, 0);
            uint8_t over = verb_findAtPos(x, y);
            if (over != 0) {
                DEBUG_PRINTF("Clicked verb %s\n", verbs[over].name);
				// Verb was clicked
				runInputScript(kVerbClickArea, verbs[over].verbid);
            }
            if (y > SCREEN_TOP + SCREEN_HEIGHT + 6)
            {
            // 	// Click into V2 inventory
            // 	int object = checkV2Inventory(_mouse.x, _mouse.y);
            // 	if (object > 0)
            // 		runInputScript(kInventoryClickArea, object, 0);
            }
        }
        else if (zone == kMainVirtScreen)
        {
            // Scene was clicked
            runInputScript((zone == kMainVirtScreen) ? kSceneClickArea : kVerbClickArea, 0);
		}
	}
}

static uint8_t isScriptInUse(uint8_t id)
{
    uint8_t i = getScriptIndex(id);
    return i != MAX_SCRIPTS;
}

void checkAndRunSentenceScript(void)
{
	if (isScriptInUse(2))
        return;

	if (!sentenceNum || sentence[sentenceNum - 1].freezeCount)
		return;

	sentenceNum--;
	SentenceTab *st = &sentence[sentenceNum];

    if (st->preposition && st->objectB == st->objectA)
        return;

    scummVars[VAR_ACTIVE_VERB] = st->verb;
    scummVars[VAR_ACTIVE_OBJECT1] = st->objectA;
    scummVars[VAR_ACTIVE_OBJECT2] = st->objectB;
    scummVars[VAR_VERB_ALLOWED] = (0 != object_getVerbEntrypoint(st->objectA, st->verb));
    // DEBUG_PRINTF("Run script 2: verb=%d o1=%d o2=%d allow=%d\n",
    //     scummVars[VAR_ACTIVE_VERB], scummVars[VAR_ACTIVE_OBJECT1],
    //     scummVars[VAR_ACTIVE_OBJECT2], scummVars[VAR_VERB_ALLOWED]);

    runScript(2);
}

void runInventoryScript(uint16_t i)
{
    //void ScummEngine_v2::redrawV2Inventory()

    // not needed
    // if (scummVars[VAR_INVENTORY_SCRIPT]) {
    //     // int args[NUM_SCRIPT_LOCAL];
    //     // memset(args, 0, sizeof(args));
    //     // args[0] = i;
    //     runScript(scummVars[VAR_INVENTORY_SCRIPT]/*, 0, 0, args*/);
    // }
}
