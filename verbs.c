#include "verbs.h"
#include "graphics.h"
#include "debug.h"

VerbSlot verbs[_numVerbs];

void verb_kill(uint8_t slot)
{
    VerbSlot *vs = &verbs[slot];
    // clear old verb on screen
    if (vs->verbid)
    {
        char *s = vs->name;
        while (*s)
            *s++ = ' ';
        graphics_drawVerb(vs);
    }

    vs->verbid = 0;
    vs->curmode = 0;
}

// TODO: mode is useless?
uint8_t verb_getSlot(uint8_t verb, uint8_t mode)
{
	uint8_t i;
	for (i = 1 ; i < _numVerbs ; i++) {
		if (verbs[i].verbid == verb/* && _verbs[i].saveid == mode*/) {
			return i;
		}
	}
	return 0;
}

uint8_t verb_findAtPos(uint8_t x, uint8_t y)
{
    uint8_t i = _numVerbs - 1;
    while (i)
    {
        VerbSlot *vs = &verbs[i];
		if (vs->curmode == 1 && vs->verbid && y == vs->y
            && x >= vs->x)
        {
            int8_t d = x - vs->x;
            char *c = vs->name;
            while (d && *c)
            {
                --d;
                ++c;
            }
            if (!d && *c)
                break;
        }

        --i;
    }
    return i;
}
