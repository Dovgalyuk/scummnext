#ifndef ACTOR_H
#define ACTOR_H

#include <stdint.h>

void actors_walk(void);

uint8_t actor_getX(uint8_t actor);
uint8_t actor_isMoving(uint8_t actor);

void actor_setRoom(uint8_t actor, uint8_t room);
void actor_setCostume(uint8_t actor, uint8_t costume);
void actor_put(uint8_t actor, uint8_t x, uint8_t y);
void actor_talk(uint8_t actor, const char *s);
void actor_startWalk(uint8_t actor, uint8_t x, uint8_t y);

#endif
