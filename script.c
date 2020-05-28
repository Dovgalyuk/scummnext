#include "script.h"
#include "resource.h"
#include "debug.h"

typedef struct Frame
{
    uint8_t id;
    uint16_t offset;
} Frame;

#define STACK_SIZE 4

enum {
    PARAM_1 = 0x80,
    PARAM_2 = 0x40,
    PARAM_3 = 0x20
};

#define VAR_EGO 0
#define VAR_CAMERA_POS_X 2
#define VAR_HAVE_MSG 3
#define VAR_ROOM 4
#define VAR_OVERRIDE 5
#define VAR_MACHINE_SPEED 6
#define VAR_CHARCOUNT 7
#define VAR_ACTIVE_VERB 8
#define VAR_ACTIVE_OBJECT1 9
#define VAR_ACTIVE_OBJECT2 10
#define VAR_NUM_ACTOR 11
#define VAR_CURRENT_LIGHTS 12
#define VAR_CURRENTDRIVE 13
#define VAR_MUSIC_TIMER 17
#define VAR_VERB_ALLOWED 18
#define VAR_ACTOR_RANGE_MIN 19
#define VAR_ACTOR_RANGE_MAX 20
#define VAR_CURSORSTATE 21
#define VAR_CAMERA_MIN_X 23
#define VAR_CAMERA_MAX_X 24
#define VAR_TIMER_NEXT 25
#define VAR_SENTENCE_VERB 26
#define VAR_SENTENCE_OBJECT1 27
#define VAR_SENTENCE_OBJECT2 28
#define VAR_SENTENCE_PREPOSITION 29
#define VAR_VIRT_MOUSE_X 30
#define VAR_VIRT_MOUSE_Y 31
#define VAR_CLICK_AREA 32
#define VAR_CLICK_VERB 33
#define VAR_CLICK_OBJECT 35
#define VAR_ROOM_RESOURCE 36
#define VAR_LAST_SOUND 37
#define VAR_BACKUP_VERB 38
#define VAR_KEYPRESS 39
#define VAR_CUTSCENEEXIT_KEY 40
#define VAR_TALK_ACTOR 41

static Frame stack[STACK_SIZE];
static int8_t curScript = -1;
static uint8_t scriptBytes[4096];
uint16_t resultVarNumber;
uint8_t opcode;

static void loadScriptBytes(void)
{
    HROOM r = seekResource(&scripts[stack[curScript].id]);
    readResource(r, scriptBytes, sizeof(scriptBytes));
    closeRoom(r);
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
    DEBUG_PRINTF("   readVar %x\n", v);
    return 0;
}

static uint16_t getVar(void)
{
	return readVar(fetchScriptByte());
}

void writeVar(uint16_t var, uint16_t val)
{
    DEBUG_PRINTF("   writeVar %x = %x\n", var, val);
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
    DEBUG_PUTS("Stopping the script\n");
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

static void pushScript(uint8_t s)
{
    DEBUG_PRINTF("Starting script %u\n", s);
    ++curScript;
    if (curScript >= STACK_SIZE - 1)
    {
        DEBUG_PUTS("Can't execute more scripts\n");
        DEBUG_HALT;
    }
    stack[curScript].id = s;
    loadScriptBytes();
    stack[curScript].offset = 4;
}

static void parseString(void)
{
    // TODO
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

		if (insertSpace)
            DEBUG_PUTC(' ');
			//*ptr++ = ' ';

	}
	//*ptr = 0;

    // TODO
	// int textSlot = 0;
	// _string[textSlot].xpos = 0;
	// _string[textSlot].ypos = 0;
	// _string[textSlot].right = _screenWidth - 1;
	// _string[textSlot].center = false;
	// _string[textSlot].overhead = false;

	//actorTalk(buffer);
}

///////////////////////////////////////////////////////////
// opcodes
///////////////////////////////////////////////////////////

static void op_resourceRoutines(void)
{
    DEBUG_PUTS("resourceRoutines\n");
	uint8_t resid = getVarOrDirectByte(PARAM_1);
	uint8_t opcode = fetchScriptByte();
    DEBUG_PRINTF("   resid %u opcode %u\n", resid, opcode);
    // TODO
}

static void op_move(void)
{
    DEBUG_PUTS("move\n");
	getResultPos();
	setResult(getVarOrDirectWord(PARAM_1));
}

static void op_setBitVar(void)
{
    DEBUG_PUTS("setBitVar\n");
	uint16_t var = fetchScriptWord();
	uint8_t a = getVarOrDirectByte(PARAM_1);

	uint16_t bit_var = var + a;
	uint8_t bit_offset = bit_var & 0x0f;
	bit_var >>= 4;

    // TODO
	if (getVarOrDirectByte(PARAM_2))
		;//_scummVars[bit_var] |= (1 << bit_offset);
	else
		;//_scummVars[bit_var] &= ~(1 << bit_offset);
}

static void op_actorOps(void)
{
    DEBUG_PUTS("actorOps\n");
    // TODO

	uint8_t act = getVarOrDirectByte(PARAM_1);
	uint8_t arg = getVarOrDirectByte(PARAM_2);
    uint8_t i;
	// Actor *a;

	uint8_t opcode = fetchScriptByte();
	if (act == 0 && opcode == 5) {
		// This case happens in the Zak/MM bootscripts, to set the default talk color (9).
		//_string[0].color = arg;
        DEBUG_PRINTF("   Set default talk color %u\n", arg);
		return;
	}

	// a = derefActor(act, "actorOps");

	switch (opcode) {
	case 1:		// SO_SOUND
		//a->_sound[0] = arg;
        DEBUG_PRINTF("   Set actor %u sound %u\n", act, arg);
		break;
	case 2:		// SO_PALETTE
        i = fetchScriptByte();
		//a->setPalette(i, arg);
        DEBUG_PRINTF("   Set actor %u palette %u/%u\n", act, i, arg);
		break;
	case 3:		// SO_ACTOR_NAME
		//loadPtrToResource(rtActorName, a->_number, NULL);
        DEBUG_PRINTF("   Set actor %u talk name '", act);
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
		//a->setActorCostume(arg);
        DEBUG_PRINTF("   Set actor %u costume %u\n", act, arg);
		break;
	case 5:		// SO_TALK_COLOR
        //a->_talkColor = arg;
        DEBUG_PRINTF("   Set actor %u talk color %u\n", act, arg);
		break;
	default:
		DEBUG_PRINTF("   Opcode %u not yet supported\n", opcode);
        DEBUG_HALT;
	}
}

static void op_print(void)
{
    uint8_t act = getVarOrDirectByte(PARAM_1);
    DEBUG_PRINTF("print(%u, ", act);
    parseString();
    DEBUG_PUTS(")\n");
}

static void op_setVarRange()
{
	uint16_t a, b;

    DEBUG_PUTS("setVarRange\n");

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

static void op_cutscene(void)
{
    DEBUG_PUTS("cutscene\n");
    // TODO
}

static void op_startScript(void)
{
    DEBUG_PUTS("startScript\n");
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
    // // Script numbers are different in V0
    // if (_game.version == 0 && script == 150) {
    //     if (VAR(104) == 1)
    //         return;
    // }
    pushScript(script);
}

static void op_beginOverride(void)
{
    DEBUG_PUTS("beginOverride\n");
	// vm.cutScenePtr[0] = _scriptPointer - _scriptOrgPointer;
	// vm.cutSceneScript[0] = _currentScript;

	// Skip the jump instruction following the override instruction
	fetchScriptByte();
	fetchScriptWord();
}

static void op_cursorCommand(void)
{
    DEBUG_PUTS("cursorCommand\n");
	uint16_t cmd = getVarOrDirectWord(PARAM_1);
    // TODO
	// byte state = cmd >> 8;

	// if (cmd & 0xFF) {
	// 	VAR(VAR_CURSORSTATE) = cmd & 0xFF;
	// }

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

static void op_loadRoom(void)
{
    uint8_t room = getVarOrDirectByte(PARAM_1);
    DEBUG_PRINTF("loadRoom %u\n", room);

    // TODO
    // startScene(room, 0, 0);
    // _fullRedraw = true;
}

void executeScript(void)
{
    while (curScript >= 0)
    {
        opcode = fetchScriptByte();
        switch (opcode)
        {
        case 0x00:
            stopScript();
            break;
        case 0x0c:
            op_resourceRoutines();
            break;
        case 0x13:
        case 0x53:
            op_actorOps();
            break;
        case 0x14:
            op_print();
            break;
        case 0x1a:
            op_move();
            break;
        case 0x1b:
        case 0x5b:
            op_setBitVar();
            break;
        case 0x26:
            op_setVarRange();
            break;
        case 0x40:
            op_cutscene();
            break;
        case 0x42:
            op_startScript();
            break;
        case 0x58:
            op_beginOverride();
            break;
        case 0x60:
            op_cursorCommand();
            break;
        case 0x64:
            op_loadRoomWithEgo();
            break;
        case 0x72:
            op_loadRoom();
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
    pushScript(s);
    executeScript();
}
