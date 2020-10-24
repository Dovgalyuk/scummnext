#ifndef ACTOR_H
#define ACTOR_H

#include <stdint.h>

#define MAX_FRAMES 4

typedef struct Animation {
    uint8_t frames;
    uint8_t anchors[MAX_FRAMES];
    uint8_t curpos;
    // anchor sprite of the current view
    uint8_t anchor;
    int8_t ax, ay;
} Animation;

typedef struct Actor
{
    uint8_t room;
    uint8_t x, y;
    
    // movement parameters
    uint8_t destX, destY;
    uint8_t nextX, nextY;
    uint8_t curX, curY;
   	int32_t deltaXFactor, deltaYFactor;
    uint8_t moving;
    uint8_t walkbox;
    uint8_t destbox;
    uint8_t curbox;

    // costume parameters
    uint8_t costume;
    // costume frame
    uint8_t frame;
    // animation of the frame
    Animation anim;
} Actor;

#define ACTOR_COUNT 25
extern Actor actors[];

void actors_walk(void);
void actors_animate(void);

uint8_t actor_getX(uint8_t actor);
uint8_t actor_isMoving(uint8_t actor);

void actor_setRoom(uint8_t actor, uint8_t room);
void actor_setCostume(uint8_t actor, uint8_t costume);
void actor_put(uint8_t actor, uint8_t x, uint8_t y);
void actor_talk(uint8_t actor, const char *s);
void actor_startWalk(uint8_t actor, uint8_t x, uint8_t y);
void actor_animate(uint8_t actor, uint8_t anim);

#endif
