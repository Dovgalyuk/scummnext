#include "camera.h"
#include "graphics.h"
#include "room.h"
#include "actor.h"
#include "debug.h"
#include "script.h"

enum {
	kNormalCameraMode = 1,
	kFollowActorCameraMode = 2,
	kPanningCameraMode = 3
};

uint8_t cameraX;
static uint8_t cameraDestX;
static uint8_t cameraMode;
static uint8_t cameraMovingToActor;
static uint8_t cameraFollows;

void camera_setX(uint8_t x)
{
    if (x < roomWidth / 2)
        x = roomWidth / 2;
    uint8_t max = roomWidth - SCREEN_WIDTH / 2;
    if (x > max)
        x = max;

	// if (camera._mode != kFollowActorCameraMode || ABS(pos_x - camera._cur.x) > (_screenWidth / 2)) {
	// 	camera._cur.x = pos_x;
	// }
	// camera._dest.x = pos_x;
    cameraX = x;
    cameraDestX = x;
    cameraMode = kNormalCameraMode;
    cameraMovingToActor = 0;
    scummVars[VAR_CAMERA_POS_X] = x;
    // TODO: min, max, destination
}

void camera_panTo(uint8_t x)
{
	cameraDestX = x;
	cameraMode = kPanningCameraMode;
	cameraMovingToActor = 0;
}

void camera_followActor(uint8_t actor)
{
    DEBUG_PRINTF("Camera is following actor %u\n", actor);
    cameraMode = kFollowActorCameraMode;
    cameraFollows = actor;

	if (actors[actor].room != currentRoom)
    {
		startScene(actors[actor].room);
		cameraX = actors[actor].x;
		camera_setX(cameraX);
	}

	// t = a->getPos().x / 8 - _screenStartStrip;

	// if (t < camera._leftTrigger || t > camera._rightTrigger || setCamera == true)
	// 	setCameraAt(a->getPos().x, 0);

	// for (i = 1; i < _numActors; i++) {
	// 	if (_actors[i]->isInCurrentRoom())
	// 		_actors[i]->_needRedraw = true;
	// }
	// runInventoryScript(0);
}

void camera_move(void)
{
    if (cameraMode == kFollowActorCameraMode)
    {
        cameraDestX = actor_getX(cameraFollows);
    }

    if (cameraX < cameraDestX/* && cameraX < roomWidth - SCREEN_WIDTH / 2*/)
        ++cameraX;
    else if (cameraX > cameraDestX/* && cameraX > SCREEN_WIDTH / 2*/)
        --cameraX;

    scummVars[VAR_CAMERA_POS_X] = cameraX;
}
