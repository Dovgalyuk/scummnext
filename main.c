#include <arch/zxn.h>
#include <arch/zxn/esxdos.h>
#include <stdio.h>
#include "debug.h"
#include "resource.h"
#include "script.h"

const int _numGlobalObjects = 775;
const int _numRooms = 55;

static const int v1MMNEScostTables[2][6] = {
	/* desc lens offs data  gfx  pal */
	{ 25,  27,  29,  31,  33,  35},
	{ 26,  28,  30,  32,  34,  36}
};

// costumes 25-36 are special. see v1MMNEScostTables[] in costume.cpp
// costumes 37-76 are room graphics resources
// costume 77 is a character set translation table
// costume 78 is a preposition list
// costume 79 is unused but allocated, so the total is a nice even number :)

int _numVariables = 800;				// 800
int _numBitVariables = 4096;			// 2048
int _numLocalObjects = 200;				// 200
int _numArray = 50;
int _numVerbs = 100;
int _numNewNames = 50;
int _numCharsets = 9;					// 9
int _numInventory = 80;					// 80
// all scripts are global in MM
//int _numGlobalScripts = 200;
int _numFlObject = 50;

int _shadowPaletteSize = 256;

uint16_t getWord(uint8_t *p)
{
    return p[0] + p[1] * 0x100;
}

// not for sound
// uint8_t *loadResource(Resource *res)
// {
//     // if (res.resource)
//     //     return res.resource;
//     HFILE f = openRoom(res->room);
//     esx_f_seek(f, res->roomoffs, ESX_SEEK_SET);

//     // old bundle
//     uint16_t size = readWord(f);
//     esx_f_seek(f, 2, ESX_SEEK_BWD);

//     // read buffer
//     uint8_t *buf = malloc(size);
//     esx_f_read(f, buf, size);

//     closeRoom(f);

//     return buf;
// }

// uint8_t *loadCostume(int n)
// {
//     return loadResource(&costumes[n]);
// }

// void decodeNESTrTable()
// {
//  	byte *table = loadCostume(77);
//     int size = getUint16LE(table);
//     printf("NES translation table size %d\n", size);
//     for (int i = 0 ; i < size ; ++i)
//         printf("char '%c' to tile %x\n", i + 32, table[i + 2]);
// }

void decodeNESTileData(uint8_t *dst, HROOM f, uint16_t len)
{
    // decode NES tile data
    uint8_t count = readByte(f);
    uint8_t data = readByte(f);
    len -= 2;
	while (len) {
		for (uint8_t j = 0; j < (count & 0x7F); j++)
        {
			*dst++ = data;
            if (count & 0x80)
            {
                if (len)
                {
                    data = readByte(f);
                    --len;
                }
            }
        }
		if (count & 0x80)
        {
            count = data;
        }
        else
        {
            if (len)
            {
                count = readByte(f);
                --len;
            }
        }
        if (len)
        {
            data = readByte(f);
            --len;
        }
	}
}

void drawPixel(uint8_t x, uint8_t y, uint8_t c)
{
    y &= 0x3f;
    *(unsigned char*)((y << 8) | x) = c;
}

void decodeNESBaseTiles()
{
    uint8_t base[4096];
    HROOM f = seekResource(&costumes[37]);
    uint16_t len = readWord(f);
    uint8_t count = readByte(f);
    DEBUG_PRINTF("NES base tiles %u %u\n", count, len);
    decodeNESTileData(base, f, len - 3);
    closeRoom(f);
 
    // print tiles
    for (uint8_t c = 0 ; c < count ; ++c)
    {
        //printf("tile %u\n", c);
        for (uint8_t i = 0 ; i < 8 ; ++i)
        {
            uint8_t c0 = base[c * 16 + i];
            uint8_t c1 = base[c * 16 + i + 8];
            //printf(" %u %u\n", c0, c1);
            for (uint8_t j = 0 ; j < 8 ; ++j)
            {
                uint8_t col = ((c0 >> (7 - j)) & 1) + (((c1 >> (7 - j)) & 1) << 1);
                uint8_t cc = col * 64;
                drawPixel(c % 32 * 8 + j, c / 32 * 8 + i, cc);
                //printf("%u ", col);
            }
            //printf("\n");
        }
    }
}

__sfr __at 0x15 IO_15;
__sfr __at 0x40 IO_40;
__sfr __at 0x41 IO_41;
__sfr __at 0x43 IO_43;
__sfr __at 0x68 IO_68;

int main()
{
    int i;
    HROOM f = openRoom(0);

    uint16_t magic = readWord(f);
	DEBUG_PRINTF("Magic %x\n", magic); /* version magic number */
	for (i = 0; i != _numGlobalObjects; i++) {
	 	uint8_t tmp = readByte(f);
        //printf("global %u owner %u state %u\n", i, tmp & 15, tmp >> 4);

	// 	_objectOwnerTable[i] = tmp & OF_OWNER_MASK;
	// 	_objectStateTable[i] = tmp >> OF_STATE_SHL;
	}

    // room offsets are always 0, skip them
    // 3 - roomno + roomoffs size
    esx_f_seek(f, _numRooms * 3, ESX_SEEK_FWD);

	for (i = 0; i < _numCostumes; i++) {
        costumes[i].room = readByte(f);
	}
	for (i = 0; i < _numCostumes; i++) {
        costumes[i].roomoffs = readWord(f);
        //printf("costume %u room %u offs %u\n", i, costumes[i].room, costumes[i].roomoffs);
	}

	for (i = 0; i < _numScripts; i++) {
        scripts[i].room = readByte(f);
	}
	for (i = 0; i < _numScripts; i++) {
        scripts[i].roomoffs = readWord(f);
        //DEBUG_PRINTF("script %u room %u offs %x\n", i, scripts[i].room, scripts[i].roomoffs);
	}

    closeRoom(f);

    // decodeNESBaseTiles();
    // while (1);

    // enable Layer 2 mode

    // setup palette
    IO_43 = 0x10;
    IO_40 = 0;
    for (i = 0 ; i < 256 ; ++i)
        IO_41 = i;

    // // switch on ULANext
    // IO_43 = 1;
    // // disable ULA output
    // IO_68 = 0x80;
    // // sprite control: sprites/layer2/ula
    // IO_15 = 1;
    // // map write only to 0-3fff
    
    // enable layer2
    IO_LAYER_2_CONFIG = 2 | 1;

    unsigned char *p = 0;
    for (i = 0 ; i < 0x4000 ; ++i)
        *p++ = i;

    // NES charset
    // decodeNESTrTable();
    decodeNESBaseTiles();

    // boot script
    runScript(1);

    while (1);
}
