Mini-SCUMMVM for ZXNext.

Supports only Maniac Mansion from NES.

Memory map:

* 0x0000 - Data buffers (pages 2,3)
* 0x4000 - Tiles
* 0x6000 - Tilemap
* 0x6a00 - code

* Character set translation table (costume 77)
* Preposition list (costume 78)
* NES base tiles (costume 37)
* Resource heap
* Costume rooms/offsets
* Script rooms/offsets

Pages map:

* 2 - tile data
* 3 - object data
* 32-43 - scripts
* 44, 45 - costumes 31 and 32 with sprite data

Resource info:

Rooms: 55: 0-54

costumes 25-36 are special. see v1MMNEScostTables[] in costume.cpp
costumes 37-76 are room graphics resources
costume 77 is a character set translation table
costume 78 is a preposition list
costume 79 is unused but allocated, so the total is a nice even number :)

static data:

extern uint8_t translationTable[256];

// costume set
extern uint8_t costdesc[51];
extern uint8_t costlens[279];
extern uint8_t costoffs[556];

