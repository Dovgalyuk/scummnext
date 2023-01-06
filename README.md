Mini-SCUMMVM for ZXNext.

Supports only Maniac Mansion from NES.

Directory structure for running:
* scummnext.nex
* MM
  * unpacked *.LFL from NES image


Memory map:

* 0x0000 - ROM, Also used for temporary mapping of other pages
* 0x4000 - Page for mapping data
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
* 3 - object data, some graphics data
* 32-47 - scripts
* 48, 49 - costumes 31 and 32 with sprite data

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


Z88DK:

https://github.com/Dovgalyuk/z88dk/tree/scumm

 z88dk/build.sh
 z88dk/libsrc/_DEVELOPMENT/make.sh
