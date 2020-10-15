#include "actor.h"
#include "graphics.h"
#include "debug.h"
#include "room.h"
#include "costume.h"
#include "box.h"
#include "helper.h"

Actor actors[ACTOR_COUNT];

enum MoveFlags {
	MF_NEW_LEG = 1,
	MF_IN_LEG = 2,
	MF_TURN = 4,
	MF_LAST_LEG = 8,
	MF_FROZEN = 0x80
};


///////////////////////////////////////////////////////////////////////////////
// Actor internals
///////////////////////////////////////////////////////////////////////////////


static void actor_walkStep(Actor *a)
{
    const uint32_t speedy = 1;
    const uint32_t speedx = 1;
	int8_t diffX, diffY;
    // TODO: split the path as in scummvm
    //       
    uint8_t nextX = a->destX;
    uint8_t nextY = a->destY;
	int32_t deltaXFactor, deltaYFactor;

	diffX = nextX - a->x;
	diffY = nextY - a->y;
	deltaYFactor = speedy << 16;

    a->moving |= MF_IN_LEG;

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

    //DEBUG_PRINTF("factorX=%l factorY=%l\n", deltaXFactor, deltaYFactor);

    //DEBUG_PRINTF("Moving diff %x %x\n", diffX, diffY);

    // _targetFacing = getAngleFromPos(V12_X_MULTIPLIER*deltaXFactor, V12_Y_MULTIPLIER*deltaYFactor, false);

	uint8_t distX = ABS(diffX);
	uint8_t distY = ABS(diffY);

    if (deltaXFactor != 0)
    {
        if (deltaXFactor > 0)
            a->x += 1;
        else
            a->x -= 1;
    }
    if (deltaYFactor != 0)
    {
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

    // doesn't work on startup, fix it later
    //costume_updateAll();
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

    costume_updateAll();
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
    DEBUG_PRINTF("Walk actor %u to %u, %u\n", actor, x, y);
    // abr = adjustXYToBeInBox(destX, destY);

	// if (!isInCurrentRoom() && _vm->_game.version <= 6) {
	// 	_pos.x = abr.x;
	// 	_pos.y = abr.y;
	// 	if (!_ignoreTurns && dir != -1)
	// 		_facing = dir;
	// 	return;
	// }

    // TODO: adjust coordinates

    uint8_t xx = x;
    uint8_t yy = y;
    uint8_t box = boxes_adjust_xy(&xx, &yy);
    DEBUG_PRINTF("Dest box %u x=%u y=%u\n", box, xx, yy);
	// if (_vm->_game.version <= 2) {
	// 	abr = adjustXYToBeInBox(abr.x, abr.y);
	// 	if (_pos.x == abr.x && _pos.y == abr.y && (dir == -1 || _facing == dir))
	// 		return;

	actors[actor].destX = x;
	actors[actor].destY = y;
    actors[actor].moving = MF_NEW_LEG;
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
        {
            actor_walkStep(actors + i);
        }
    }
}

void actor_animate(uint8_t actor, uint8_t anim)
{
    // TODO: different commands and directions from void Actor::animateActor(int anim)

    // then void Actor::startAnimActor(int f = anim/4)
    // _animProgress = 0;
    // _needRedraw = true;
    // _cost.animCounter = 0;
	actors[actor].frame = anim / 4;
    actors[actor].anim.curpos = 0;

    costume_updateAll();
}

void actors_animate(void)
{
    uint8_t i;
    for (i = 0 ; i < ACTOR_COUNT ; ++i)
    {
        // TODO: any other flags?
        if (actors[i].room == currentRoom)
        {
            ++actors[i].anim.curpos;
            if (actors[i].anim.curpos >= actors[i].anim.frames)
                actors[i].anim.curpos = 0;
        }
    }
}
