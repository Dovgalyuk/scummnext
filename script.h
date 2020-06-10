#ifndef SCRIPT_H
#define SCRIPT_H

#include <stdint.h>

#include "room.h"

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

extern uint16_t scummVars[];

void runScript(uint8_t s);
void runRoomScript(uint8_t room, uint16_t offs);
void processScript(void);

#endif
