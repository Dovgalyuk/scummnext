#include "verbs.h"
#include "debug.h"

VerbSlot verbs[_numVerbs];

void verb_kill(uint8_t slot)
{
    verbs[slot].verbid = 0;
    verbs[slot].curmode = 0;
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
