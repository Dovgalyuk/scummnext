#include "actor.h"
#include "graphics.h"
#include "debug.h"
#include "room.h"

#define ACTOR_COUNT 25
typedef struct Actor
{
    uint8_t room;
    uint8_t x, y;
    uint8_t destX, destY;
    uint8_t moving;
    uint8_t costume;
} Actor;

static Actor actors[ACTOR_COUNT];

///////////////////////////////////////////////////////////////////////////////
// Actor internals
///////////////////////////////////////////////////////////////////////////////

#define ABS(x) ((x) < 0 ? -(x) : (x))

static void actor_walkStep(Actor *a)
{
    const uint32_t speedy = 1;
    const uint32_t speedx = 1;
	int8_t diffX, diffY;
    // TODO: path
    uint8_t nextX = a->destX;
    uint8_t nextY = a->destY;
	int32_t deltaXFactor, deltaYFactor;

	diffX = nextX - a->x;
	diffY = nextY - a->y;
	deltaYFactor = speedy << 16;

	if (diffY < 0)
		deltaYFactor = -deltaYFactor;

	deltaXFactor = deltaYFactor * diffX;
	if (diffY != 0) {
		deltaXFactor /= diffY;
	} else {
		deltaYFactor = 0;
	}

	if (ABS(deltaXFactor) > (speedx << 16))	{
		deltaXFactor = speedx << 16;
		if (diffX < 0)
			deltaXFactor = -deltaXFactor;

		deltaYFactor = deltaXFactor * diffY;
		if (diffX != 0) {
			deltaYFactor /= diffX;
		} else {
			deltaXFactor = 0;
		}
	}

    // _targetFacing = getAngleFromPos(V12_X_MULTIPLIER*deltaXFactor, V12_Y_MULTIPLIER*deltaYFactor, false);

	uint8_t distX = ABS(diffX);
	uint8_t distY = ABS(diffY);

	// if (ABS(_pos.x - _walkdata.cur.x) >= distX && ABS(_pos.y - _walkdata.cur.y) >= distY) {
	// 	_moving &= ~MF_IN_LEG;
	// 	return 0;
	// }

    if (deltaXFactor != 0) {
        if (deltaXFactor > 0)
            a->x += 1;
        else
            a->x -= 1;
    }
    if (deltaYFactor != 0) {
        if (deltaYFactor > 0)
            a->y += 1;
        else
            a->y -= 1;
    }

    if (distX == 0 && distY == 0)
    {
        // TODO: moving phase support
        a->moving = 0;
    }

	// if (ABS(a->x - _walkdata.cur.x) > distX) {
	// 	a->x = _walkdata.next.x;
	// }

	// if (ABS(a->y - _walkdata.cur.y) > distY) {
	// 	a->y = _walkdata.next.y;
	// }

	// if ((_vm->_game.version <= 2 || (_vm->_game.version >= 4 && _vm->_game.version <= 6)) && _pos == _walkdata.next) {
	// 	_moving &= ~MF_IN_LEG;
	// 	return 0;
	// }
}

///////////////////////////////////////////////////////////////////////////////
// Actor interface
///////////////////////////////////////////////////////////////////////////////

uint8_t actor_getX(uint8_t actor)
{
    return actors[actor].x;
}

uint8_t actor_isMoving(uint8_t actor)
{
    return actors[actor].moving;
}

void actor_setCostume(uint8_t actor, uint8_t costume)
{
    DEBUG_PRINTF("Set actor %u costume %u\n", actor, costume);
    actors[actor].costume = costume;
}

void actor_talk(uint8_t actor, const char *s)
{
    // TODO: color
    graphics_print(s);
}

void actor_setRoom(uint8_t actor, uint8_t room)
{
    DEBUG_PRINTF("Put actor %u in room %u\n", actor, room);
    actors[actor].room = room;
}

void actor_put(uint8_t actor, uint8_t x, uint8_t y)
{
    DEBUG_PRINTF("Put actor %u to %u, %u\n", actor, x, y);
    actors[actor].x = x;
    actors[actor].y = y;
    actors[actor].moving = 0;
}

void actor_startWalk(uint8_t actor, uint8_t x, uint8_t y)
{
    // abr = adjustXYToBeInBox(destX, destY);

	// if (!isInCurrentRoom() && _vm->_game.version <= 6) {
	// 	_pos.x = abr.x;
	// 	_pos.y = abr.y;
	// 	if (!_ignoreTurns && dir != -1)
	// 		_facing = dir;
	// 	return;
	// }

	// if (_vm->_game.version <= 2) {
	// 	abr = adjustXYToBeInBox(abr.x, abr.y);
	// 	if (_pos.x == abr.x && _pos.y == abr.y && (dir == -1 || _facing == dir))
	// 		return;

	actors[actor].destX = x;
	actors[actor].destY = y;
    actors[actor].moving = 1;
	// _walkdata.destbox = abr.box;
	// _walkdata.destdir = dir;
	// _walkdata.point3.x = 32000;
	// _walkdata.curbox = _walkbox;
    //_moving = (_moving & ~(MF_LAST_LEG | MF_IN_LEG)) | MF_NEW_LEG;
}

void actors_walk(void)
{
    uint8_t i;
    for (i = 0 ; i < ACTOR_COUNT ; ++i)
    {
        if (actors[i].room == currentRoom && actors[i].moving)
            actor_walkStep(actors + i);
    }
}
