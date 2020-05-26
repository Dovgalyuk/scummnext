#include <arch/zxn.h>
#include <arch/zxn/esxdos.h>
#include <stdio.h>

const int _numGlobalObjects = 775;
const int _numRooms = 55;

// costumes 25-36 are special. see v1MMNEScostTables[] in costume.cpp
// costumes 37-76 are room graphics resources
// costume 77 is a character set translation table
// costume 78 is a preposition list
// costume 79 is unused but allocated, so the total is a nice even number :)
const int _numCostumes = 80;
const int _numScripts = 200;
const int _numSounds = 100;

int _numVariables = 800;				// 800
int _numBitVariables = 4096;			// 2048
int _numLocalObjects = 200;				// 200
int _numArray = 50;
int _numVerbs = 100;
int _numNewNames = 50;
int _numCharsets = 9;					// 9
int _numInventory = 80;					// 80
int _numGlobalScripts = 200;
int _numFlObject = 50;

int _shadowPaletteSize = 256;


#define ENC_BYTE 0xff
typedef unsigned char HFILE;

uint8_t readByte(HFILE f)
{
    uint8_t b;
    esx_f_read(f, &b, 1);
    return b ^ ENC_BYTE;
}

uint16_t readWord(HFILE f)
{
    uint8_t b[2];
    esx_f_read(f, b, 2);
    return (b[0] ^ ENC_BYTE) + (b[1] ^ ENC_BYTE) * 0x100;
}


int main()
{
    unsigned char f = esx_f_open("MM/00.LFL", ESX_MODE_OPEN_EXIST | ESX_MODE_READ);

	printf("Magic %u\n", readWord(f)); /* version magic number */
	for (int i = 0; i != _numGlobalObjects; i++) {
	 	uint8_t tmp = readByte(f);
        printf("global %u owner %u state %u\n", i, tmp & 15, tmp >> 4);

	// 	_objectOwnerTable[i] = tmp & OF_OWNER_MASK;
	// 	_objectStateTable[i] = tmp >> OF_STATE_SHL;
	}

    while (1);
}
