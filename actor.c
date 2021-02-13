#include "actor.h"
#include "graphics.h"
#include "debug.h"
#include "room.h"
#include "costume.h"
#include "box.h"
#include "helper.h"
#include "object.h"
#include "script.h"
#include "string.h"

#define _standFrame    1
#define _initFrame     2
#define _talkStopFrame 4

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

static void actor_setBox(Actor *a, uint8_t box)
{
    a->walkbox = box;
    // setupActorScale();
    DEBUG_PRINTF("Set box %u\n", box);
}

static void actor_calcMovementFactor(Actor *a, uint8_t nextX, uint8_t nextY)
{
    const uint32_t speedy = 1;
    const uint32_t speedx = 1;
    int8_t diffX, diffY;

    if (a->x == nextX && a->y == nextY)
        return;

    diffX = nextX - a->x;
    diffY = nextY - a->y;
    a->deltaYFactor = speedy << 16;

    if (diffY < 0)
        a->deltaYFactor = -a->deltaYFactor;

    a->deltaXFactor = a->deltaYFactor * diffX;
    if (diffY != 0) {
        a->deltaXFactor /= diffY;
    } else {
        a->deltaYFactor = 0;
    }

    if (ABS(a->deltaXFactor) > (speedx << 16))	{
        a->deltaXFactor = speedx << 16;
        if (diffX < 0)
            a->deltaXFactor = -a->deltaXFactor;

        a->deltaYFactor = a->deltaXFactor * diffY;
        if (diffX != 0) {
            a->deltaYFactor /= diffX;
        } else {
            a->deltaXFactor = 0;
        }
    }

    a->curX = a->x;
    a->curY = a->y;
    a->nextX = nextX;
    a->nextY = nextY;

    // _targetFacing = getAngleFromPos(V12_X_MULTIPLIER*deltaXFactor, V12_Y_MULTIPLIER*deltaYFactor, false);
}

static void actor_walkStep(Actor *a)
{
    // nextFacing = updateActorDirection(true);
    if (!(a->moving & MF_IN_LEG)/* || _facing != nextFacing*/)
    {
    //     if (_walkFrame != _frame || _facing != nextFacing) {
    //         startWalkAnim(1, nextFacing);
    //     }
        a->moving |= MF_IN_LEG;
    }

    if (a->walkbox != a->curbox && box_checkXYInBounds(a->curbox, a->x, a->y))
    {
    	actor_setBox(a, a->curbox);
    }

    //DEBUG_PRINTF("factorX=%l factorY=%l\n", deltaXFactor, deltaYFactor);

    // _targetFacing = getAngleFromPos(V12_X_MULTIPLIER*deltaXFactor, V12_Y_MULTIPLIER*deltaYFactor, false);

	uint8_t distX = ABS(a->nextX - a->curX);
	uint8_t distY = ABS(a->nextY - a->curY);

	if (ABS(a->x - a->curX) >= distX && ABS(a->y - a->curY) >= distY) {
		a->moving &= ~MF_IN_LEG;
		return;
	}

    // DEBUG_PRINTF("Moving diff %d %d\n", diffX, diffY);

    if (a->deltaXFactor != 0)
    {
        if (a->deltaXFactor > 0)
            a->x += 1;
        else
            a->x -= 1;
    }
    if (a->deltaYFactor != 0)
    {
        if (a->deltaYFactor > 0)
            a->y += 1;
        else
            a->y -= 1;
    }

    if (ABS(a->x - a->curX) > distX) {
        a->x = a->nextX;
    }

    if (ABS(a->y - a->curY) > distY) {
        a->y = a->nextY;
    }

    if (a->x == a->nextX && a->y == a->nextY)
        a->moving &= ~MF_IN_LEG;
}

static void actor_walk(Actor *a)
{
    uint8_t foundX, foundY;
    int8_t next_box;
    // Common::Point foundPath, tmp;
    // int new_dir, next_box;

    // if (_moving & MF_TURN) {
    //     new_dir = updateActorDirection(false);
    //     if (_facing != new_dir) {
    //         setDirection(new_dir);
    //     } else {
    //         _moving = 0;
    //     }
    //     return;
    // }

    if (a->moving & MF_IN_LEG) {
        actor_walkStep(a);
    } else {
        if (a->moving & MF_LAST_LEG) {
            a->moving = 0;
            // startAnimActor(_standFrame);
            // if (_targetFacing != _walkdata.destdir)
            //     turnToDirection(_walkdata.destdir);
        } else {
            actor_setBox(a, a->curbox);
            if (a->walkbox == a->destbox) {
                foundX = a->destX;
                foundY = a->destY;
                a->moving |= MF_LAST_LEG;
            } else {
                next_box = boxes_getNext(a->walkbox, a->destbox);
                if (next_box < 0) {
                    a->moving |= MF_LAST_LEG;
                    return;
                }

                // Can't walk through locked boxes
                // int flags = _vm->getBoxFlags(next_box);
                // if ((flags & kBoxLocked) && !((flags & kBoxPlayerOnly) && !isPlayer())) {
                //     a->moving |= MF_LAST_LEG;
                // }

                a->curbox = next_box;

                uint8_t tx, ty;
                box_getClosestPtOnBox(a->curbox, a->x, a->y, &tx, &ty);
                box_getClosestPtOnBox(a->walkbox, tx, ty, &foundX, &foundY);
            }
            actor_calcMovementFactor(a, foundX, foundY);
            actor_walkStep(a);
        }
    }
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

void actor_stopTalk(void)
{
    scummVars[VAR_HAVE_MSG] = 0;
    talkDelay = 0;
}

void actor_talk(uint8_t actor, const uint8_t *s)
{
    // TODO: color
    // TODO: variables

    scummVars[VAR_HAVE_MSG] = 0xff;

    message_print(s);
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
    Actor *a = &actors[actor];
    a->moving = 0;
    a->x = x;
    a->y = y;
    // if (_visible && _vm->_currentRoom != newRoom && _vm->getTalkingActor() == _number) {
    actor_stopTalk();
    if (actor_isInCurrentRoom(actor))
    {
        uint8_t box = boxes_adjustXY(&x, &y);
        a->x = x;
        a->y = y;
        a->destbox = box;
        actor_setBox(a, box);
    }
}

void actor_startWalk(uint8_t actor, uint8_t x, uint8_t y)
{
    DEBUG_PRINTF("Walk actor %u to %u, %u\n", actor, x, y);

    // TODO: jump to this room

	// if (!isInCurrentRoom() && _vm->_game.version <= 6) {
	// 	_pos.x = abr.x;
	// 	_pos.y = abr.y;
	// 	if (!_ignoreTurns && dir != -1)
	// 		_facing = dir;
	// 	return;
	// }

    uint8_t xx = x;
    uint8_t yy = y;
    uint8_t box = boxes_adjustXY(&xx, &yy);
    DEBUG_PRINTF("Dest box %u x=%u y=%u\n", box, xx, yy);

	// 	if (_pos.x == abr.x && _pos.y == abr.y && (dir == -1 || _facing == dir))
	// 		return;
    if (actors[actor].x == xx && actors[actor].y == yy) {
        return;
    }

	actors[actor].destX = xx;
	actors[actor].destY = yy;
    actors[actor].moving = MF_NEW_LEG;
    actors[actor].destbox = box;
    actors[actor].curbox = actors[actor].walkbox;
	// _walkdata.destdir = dir;
	// _walkdata.point3.x = 32000;
    //_moving = (_moving & ~(MF_LAST_LEG | MF_IN_LEG)) | MF_NEW_LEG;
}

void actors_walk(void)
{
    uint8_t i;
    for (i = 0 ; i < ACTOR_COUNT ; ++i)
    {
        if (actor_isInCurrentRoom(i) && actors[i].moving)
        {
            actor_walk(actors + i);
        }
    }
}

static void startAnimActor(uint8_t actor, uint8_t f)
{
    // TODO

    // then void Actor::startAnimActor(int f = anim/4)
    // _animProgress = 0;
    // _needRedraw = true;
    // _cost.animCounter = 0;
    actors[actor].frame = f;
    actors[actor].anim.curpos = 0;
}


void actor_animate(uint8_t actor, uint8_t anim)
{
    // TODO: different commands and directions from void Actor::animateActor(int anim)

    uint8_t cmd = anim / 4;
    //dir = oldDirToNewDir(anim % 4);

    // Convert into old cmd code
    cmd = 0x3F - cmd + 2;

	switch (cmd) {
	case 2:				// stop walking
        DEBUG_PRINTF("TODO: Actor %u stop walking\n", actor);
		startAnimActor(actor, _standFrame);
		// stopActorMoving();
		break;
	case 3:				// change direction immediatly
        DEBUG_PRINTF("TODO: Actor %u set direction\n", actor);
		// _moving &= ~MF_TURN;
		// setDirection(dir);
		break;
	case 4:				// turn to new direction
        DEBUG_PRINTF("TODO: Actor %u turn to direction\n", actor);
		// turnToDirection(dir);
		break;
	case 64:
	default:
        startAnimActor(actor, anim / 4);
        break;
    }

    costume_updateAll();
}

void actors_animate(void)
{
    uint8_t i;
    for (i = 0 ; i < ACTOR_COUNT ; ++i)
    {
        // TODO: any other flags?
        if (actor_isInCurrentRoom(i))
        {
            ++actors[i].anim.curpos;
            if (actors[i].anim.curpos >= actors[i].anim.frames)
                actors[i].anim.curpos = 0;
        }
    }
}

void actor_walkToObject(uint8_t actor, uint16_t obj_id)
{
    Object *obj = object_get(obj_id);
	// int x, y, dir;
    uint8_t x = obj->walk_x;
    uint8_t y = obj->walk_y;
	// getObjectXYPos(obj, x, y, dir);

    // TODO: calculate direction

    boxes_adjustXY(&x, &y);
    actor_startWalk(actor, x, y);
	// Actor *a = derefActor(actor, "walkActorToObject");
	// AdjustBoxResult r = a->adjustXYToBeInBox(x, y);
	// x = r.x;
	// y = r.y;

	// a->startWalkActor(x, y, dir);
}

static void actor_show(uint8_t a)
{
    // if (_visible) return;

//	adjustActorPos();
    // _cost.reset();
    startAnimActor(a, _standFrame);
    startAnimActor(a, _initFrame);
    startAnimActor(a, _talkStopFrame);
	// stopActorMoving();
    actors[a].moving = 0;
	// _visible = true;
	// _needRedraw = true;
}

void actors_show(void)
{
    uint8_t i;
    for (i = 0 ; i < ACTOR_COUNT ; ++i)
    {
        if (actor_isInCurrentRoom(i))
        {
            actor_show(i);
        }
    }
}

uint8_t actor_isInCurrentRoom(uint8_t actor)
{
    return currentRoom && actors[actor].room == currentRoom;
}

void actor_walkToActor(uint8_t actor, uint8_t toActor, uint8_t dist)
{
	uint8_t x = actors[toActor].x;
	uint8_t y = actors[toActor].y;
	if (x < actors[actor].x)
		x += dist;
	else
		x -= dist;

    boxes_adjustXY(&x, &y);
	actor_startWalk(actor, x, y);
}
