#ifndef ACTOR_H
#define ACTOR_H

#include <stdint.h>

//#define MAX_FRAMES 8

typedef struct Actor
{
    uint8_t room;
    uint8_t x, y;

    // name
    char name[16];
    // not used for NES
    //uint8_t talkColor;

    // movement parameters
    uint8_t destX, destY;
    uint8_t nextX, nextY;
    uint8_t curX, curY;
   	int32_t deltaXFactor, deltaYFactor;
    uint8_t moving;
    uint8_t walkbox;
    uint8_t destbox;
    //int8_t destdir;
    uint8_t curbox;
    // { 270, 90, 180, 0 }
    uint8_t facing;
    uint8_t targetFacing;

    // costume parameters
    uint8_t costume;
    // costume frame
    uint8_t frame;
    // animation of the frame
    uint8_t frames;
    //uint8_t anchors[MAX_FRAMES];
    uint8_t curpos;
    // anchor sprite of the current view
    uint8_t anchor;
    uint8_t old_anchor;
    int8_t ax, ay;
} Actor;

#define ACTOR_COUNT 25
extern Actor actors[];
extern uint8_t defaultTalkColor;

void actors_walk(void);
void actors_animate(void);
void actors_show(void);
void actors_draw(uint8_t offs, uint8_t gap);

uint8_t actor_getX(uint8_t actor);
uint8_t actor_isMoving(uint8_t actor);
uint8_t actor_isInCurrentRoom(uint8_t actor);
uint8_t actor_getFromPos(uint8_t x, uint8_t y);

void actor_setRoom(uint8_t actor, uint8_t room);
void actor_setCostume(uint8_t actor, uint8_t costume);
// void actor_setTalkColor(uint8_t actor, uint8_t color);
void actor_put(uint8_t actor, uint8_t x, uint8_t y);
void actor_stopTalk(void);
void actor_talk(uint8_t actor, const uint8_t *s);
void actor_startWalk(uint8_t actor, uint8_t x, uint8_t y);
void actor_animate(uint8_t actor, uint8_t anim);
void actor_walkToObject(uint8_t actor, uint16_t obj_id);
void actor_walkToActor(uint8_t actor, uint8_t toActor, uint8_t dist);

#endif
