#ifndef CAMERA_H
#define CAMERA_H

#include <stdint.h>

extern uint8_t cameraX;

void camera_move(void);

void camera_setX(uint8_t x);
void camera_panTo(uint8_t x);
void camera_followActor(uint8_t actor);

#endif
