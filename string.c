#include "string.h"
#include "graphics.h"
#include "script.h"
#include "debug.h"

#define _defaultTalkDelay 3

static uint8_t messageBuf[128];
static uint8_t *curPos;
static uint8_t color;
int8_t talkDelay;


uint8_t *message_new(void)
{
    curPos = messageBuf;
    return messageBuf;
}

void message_print(uint8_t *m, uint8_t c)
{
    uint8_t haveMsg = 1;
    //talkDelay += _msgCount * _defaultTalkDelay;
    curPos = m;
    color = c;
    talkDelay = 0;
    scummVars[VAR_CHARCOUNT] = 0;
    while (*curPos)
    {
        talkDelay += 3;
        ++scummVars[VAR_CHARCOUNT];
        if (*curPos == 3)
        {
            // next part of the message
            *curPos = 0;
            haveMsg = 0xff;
            break;
        }
        ++curPos;
    }
    //talkDelay = 60;
    // TODO: switch between messages
    scummVars[VAR_HAVE_MSG] = haveMsg;

    graphics_print(m, color);
}

void messages_update(void)
{
    if (!scummVars[VAR_HAVE_MSG])
        return;

    if (talkDelay)
        return;

    // last message processed
    if (scummVars[VAR_HAVE_MSG] == 1)
    {
        actor_stopTalk();
        return;
    }

    // process next message
    message_print(curPos + 1, color);
}
