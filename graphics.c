#include <arch/zxn.h>
#include <arch/zxn/esxdos.h>
#include "graphics.h"
#include "resource.h"
#include "debug.h"

#define TILEMAP_BASE 0x6000
#define TILE_BASE 0x4000
#define LINE_WIDTH 40
#define LINE_BYTES (LINE_WIDTH * 2)

static uint8_t baseTilesCount;
static uint16_t tilesCount;
static uint8_t translationTable[256];

static const uint8_t tableNESPalette[] = {
    /*    0x1D     */
    0x24, 0x24, 0x24, 	0x00, 0x24, 0x92, 	0x00, 0x00, 0xDB, 	0x6D, 0x49, 0xDB,
    0x92, 0x00, 0x6D, 	0xB6, 0x00, 0x6D, 	0xB6, 0x24, 0x00, 	0x92, 0x49, 0x00,
    0x6D, 0x49, 0x00, 	0x24, 0x49, 0x00, 	0x00, 0x6D, 0x24, 	0x00, 0x92, 0x00,
    0x00, 0x49, 0x49, 	0x00, 0x00, 0x00, 	0x00, 0x00, 0x00, 	0x00, 0x00, 0x00,

    0xB6, 0xB6, 0xB6, 	0x00, 0x6D, 0xDB, 	0x00, 0x49, 0xFF, 	0x92, 0x00, 0xFF,
    0xB6, 0x00, 0xFF, 	0xFF, 0x00, 0x92, 	0xFF, 0x00, 0x00, 	0xDB, 0x6D, 0x00,
    0x92, 0x6D, 0x00, 	0x24, 0x92, 0x00, 	0x00, 0x92, 0x00, 	0x00, 0xB6, 0x6D,
                        /*    0x00     */
    0x00, 0x92, 0x92, 	0x6D, 0x6D, 0x6D, 	0x00, 0x00, 0x00, 	0x00, 0x00, 0x00,

    0xFF, 0xFF, 0xFF, 	0x6D, 0xB6, 0xFF, 	0x92, 0x92, 0xFF, 	0xDB, 0x6D, 0xFF,
    0xFF, 0x00, 0xFF, 	0xFF, 0x6D, 0xFF, 	0xFF, 0x92, 0x00, 	0xFF, 0xB6, 0x00,
    0xDB, 0xDB, 0x00, 	0x6D, 0xDB, 0x00, 	0x00, 0xFF, 0x00, 	0x49, 0xFF, 0xDB,
    0x00, 0xFF, 0xFF, 	0x49, 0x49, 0x49, 	0x00, 0x00, 0x00, 	0x00, 0x00, 0x00,

    0xFF, 0xFF, 0xFF, 	0xB6, 0xDB, 0xFF, 	0xDB, 0xB6, 0xFF, 	0xFF, 0xB6, 0xFF,
    0xFF, 0x92, 0xFF, 	0xFF, 0xB6, 0xB6, 	0xFF, 0xDB, 0x92, 	0xFF, 0xFF, 0x49,
    0xFF, 0xFF, 0x6D, 	0xB6, 0xFF, 0x49, 	0x92, 0xFF, 0x6D, 	0x49, 0xFF, 0xDB,
    0x92, 0xDB, 0xFF, 	0x92, 0x92, 0x92, 	0x00, 0x00, 0x00, 	0x00, 0x00, 0x00
};

void initGraphics(void)
{
    int i;

    // enable tilemap with attributes
    ZXN_WRITE_REG(0x6B, 0x81);
    ZXN_WRITE_REG(0x6C, 0);
    // transparency index
    //ZXN_WRITE_REG(0x4C, 0);
    // disable ULA
    ZXN_WRITE_REG(0x68, 0x80);


    // setup tilemap first palette
    ZXN_WRITE_REG(0x43, 0x30);
    // ZXN_WRITE_REG(0x40, 0);
    // for (i = 0 ; i < 256 ; ++i)
    // {
	// ZXN_WRITE_REG(0x44, 0x0b); ZXN_WRITE_REG(0x44, 0x1);//	; 0  (0) Black		
    // }
    //     ZXN_WRITE_REG(0x41, i);
    // // ZXN_WRITE_REG(0x40 = 0x4;
    // // ZXN_WRITE_REG(0x41 = 0xe0;
    // // ZXN_WRITE_REG(0x40 = 0x8;
    // // ZXN_WRITE_REG(0x41 = 0x1c;
    // // ZXN_WRITE_REG(0x40 = 0xc;
    // // ZXN_WRITE_REG(0x41 = 0x3;
    // ZXN_WRITE_REG(0x40, 0);
	// ZXN_WRITE_REG(0x44, 0x00); ZXN_WRITE_REG(0x44, 0x10);//	; 0  (0) Black		
	// ZXN_WRITE_REG(0x44, 0x0c); ZXN_WRITE_REG(0x44, 0x01);// 	; 1  (1) Green
	// ZXN_WRITE_REG(0x44, 0xfc); ZXN_WRITE_REG(0x44, 0x00);//	; 2  (2) Bright Yellow
	// ZXN_WRITE_REG(0x44, 0xa0); ZXN_WRITE_REG(0x44, 0x00);//	; 3  (3) Red
	// ZXN_WRITE_REG(0x44, 0x16); ZXN_WRITE_REG(0x44, 0x01);//	; 4  (4) Light Blue 
	// ZXN_WRITE_REG(0x44, 0xe0); ZXN_WRITE_REG(0x44, 0x00);//	; 5  (5) Bright Red
	// ZXN_WRITE_REG(0x44, 0xaa); ZXN_WRITE_REG(0x44, 0x00);//	; 6  (6) Purple
	// ZXN_WRITE_REG(0x44, 0xf0); ZXN_WRITE_REG(0x44, 0x00);//	; 7  (7) Orange
	// ZXN_WRITE_REG(0x44, 0x1c); ZXN_WRITE_REG(0x44, 0x01);//	; 8  (8) Light Green
	// ZXN_WRITE_REG(0x44, 0x8c); ZXN_WRITE_REG(0x44, 0x00);//	; 9  (9) Light Brown
	// ZXN_WRITE_REG(0x44, 0x03); ZXN_WRITE_REG(0x44, 0x00);//	; 10 (A) Blue
	// ZXN_WRITE_REG(0x44, 0x92); ZXN_WRITE_REG(0x44, 0x00);//	; 11 (B) Grey
	// ZXN_WRITE_REG(0x44, 0xf2); ZXN_WRITE_REG(0x44, 0x00);//	; 12 (C) Pink
	// ZXN_WRITE_REG(0x44, 0xfd); ZXN_WRITE_REG(0x44, 0x10);//	; 13 (D) Yellow
	// ZXN_WRITE_REG(0x44, 0xff); ZXN_WRITE_REG(0x44, 0x10);//	; 14 (E) White
    

    // tilemap base address is 0x6000
    ZXN_WRITE_REG(0x6E, TILEMAP_BASE >> 8);
    // tile base address is 0x6500
    ZXN_WRITE_REG(0x6F, TILE_BASE >> 8);

    // // switch on ULANext
    // ZXN_WRITE_REG(0x43 = 1;
    // // disable ULA output
    // ZXN_WRITE_REG(0x68 = 0x80;
    // // sprite control: sprites/layer2/ula
    // ZXN_WRITE_REG(0x15 = 1;
    // // map write only to 0-3fff
    
    // enable layer2
    // ZXN_WRITE_REG(0xLAYER_2_CONFIG = 2 | 1;

    // unsigned char *p = 0;
    // for (i = 0 ; i < 0x4000 ; ++i)
    //     *p++ = i;

    // // DEBUG_PUTS("Setting tiles\n");

    // uint8_t *p = (uint8_t*)TILE_BASE;
    // for (i = 0 ; i < 256 ; ++i)
    //     for (j = 0 ; j < 32 ; ++j)
    //         *p++ = i;

    // // DEBUG_PUTS("Setting tilemap\n");

    uint8_t *p = (uint8_t*)TILEMAP_BASE + LINE_BYTES * 20;
    for (i = 0 ; i < 256 ; ++i)
    {
        *p++ = i;
        *p++ = 0;
    }

    // // DEBUG_PUTS("End setup\n");
}

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

void decodeNESTrTable(void)
{
    HROOM r = seekResource(&costumes[77]);
    uint8_t size = readWord(r);
    DEBUG_PRINTF("NES translation table size %u\n", size);
    for (uint8_t i = 0 ; i < size - 2 ; ++i)
        translationTable[i + ' '] = readByte(r);
    closeRoom(r);
}

static void decodeNESTiles(uint8_t set)
{
    uint8_t base[4096];
    HROOM f = seekResource(&costumes[set]);
    uint16_t len = readWord(f);
    uint8_t count = readByte(f);
    DEBUG_PRINTF("NES tiles %u %u\n", count, len);
    decodeNESTileData(base, f, len - 3);
    closeRoom(f);
 
    uint8_t *tiles = (uint8_t*)TILE_BASE + baseTilesCount * 32;
    //uint8_t *screen = (uint8_t*)TILEMAP_BASE;
    if (!baseTilesCount)
        baseTilesCount = count;
    tilesCount = baseTilesCount + count;
    for (uint8_t c = 0 ; c < count ; ++c)
    {
        // DEBUG_PRINTF("Tile %u: ", c);
        for (uint8_t i = 0 ; i < 8 ; ++i)
        {
            uint8_t c0 = base[c * 16 + i];
            uint8_t c1 = base[c * 16 + i + 8];
            for (uint8_t j = 0 ; j < 8 ; ++j)
            {
                uint8_t col1 = ((c0 >> (7 - j)) & 1) + (((c1 >> (7 - j)) & 1) << 1);
                ++j;
                uint8_t col2 = ((c0 >> (7 - j)) & 1) + (((c1 >> (7 - j)) & 1) << 1);
                uint8_t b = (col1 << 4) | col2;
                *tiles++ = b;
                // DEBUG_PRINTF("%x", b);
            }
        }
        // DEBUG_PUTS("\n");
        //*screen++ = c;
    }
}

void decodeNESBaseTiles(void)
{
    decodeNESTiles(37);
}

static uint8_t nametable[16][64];
static uint8_t palette[16];
static uint8_t attributes[64];

void decodeNESGfx(HROOM r)
{
    uint8_t i, j;
    uint16_t n;
    uint16_t width;
    esx_f_seek(r, 0x4, ESX_SEEK_SET);
    width = readWord(r);
    esx_f_seek(r, 0xa, ESX_SEEK_SET);
    uint16_t gdata = readWord(r);
    esx_f_seek(r, gdata, ESX_SEEK_SET);
    uint8_t tileset = readByte(r);
    DEBUG_PRINTF("decoding gdata %u tileset %u\n", gdata, tileset);

    decodeNESTiles(37 + tileset);
    // decode palette
    for (i = 0 ; i < 16 ; ++i)
    {
        uint8_t c = readByte(r);
		if (c == 0x0D)
			c = 0x1D;
		if (c == 0x1D)	 // HACK - switch around colors 0x00 and 0x1D
			c = 0;		 // so we don't need a zillion extra checks
		else if (c == 0) // for determining the proper background color
			c = 0x1D;
        palette[i] = c;
    }
    // decode name table
    uint8_t data = readByte(r);
    for (i = 0 ; i < 16 ; ++i)
    {
        nametable[i][0] = nametable[i][1];
		n = 0;
		while (n < width)
        {
            uint8_t next = readByte(r);
			for (j = 0 ; j < (data & 0x7F) ; j++)
            {
                nametable[i][2 + n++] = next;
                if (data & 0x80)
                    next = readByte(r);
            }
			if (!(data & 0x80))
				next = readByte(r);
            data = next;
		}
		nametable[i][width + 2] = nametable[i][width + 3] = 0;
    }
    // decode attributes
    esx_f_seek(r, 0xc, ESX_SEEK_SET);
    uint16_t adata = readWord(r);
    esx_f_seek(r, adata, ESX_SEEK_SET);
    data = readByte(r);
	for (n = 0 ; n < 64 ; ) {
        uint8_t next = readByte(r);
		for (j = 0 ; j < (data & 0x7F) ; j++)
        {
			attributes[n++] = next;
            if (data & 0x80)
                next = readByte(r);
        }
		if (!(n & 7) && (width == 0x1C))
			n += 8;
		if (!(data & 0x80))
			next = readByte(r);
        data = next;
	}
    // decode masktable
}

void graphics_print(const char *s)
{
    uint8_t *screen = (uint8_t*)TILEMAP_BASE;
    while (*s)
    {
        //DEBUG_PRINTF("char %u pattern %u\n", *s, translationTable[*s]);
        *screen++ = translationTable[*s++];
        *screen++ = 0;
    }
}

void handleDrawing(void)
{
    uint8_t i, j;
    // update palette
    for (i = 0 ; i < 16 ; ++i)
    {
        // color(0-3) + palette offset(0-3)
        ZXN_WRITE_REG(0x40, (i & 3) | ((i & 0xc) << 2));
        uint8_t c = palette[i] * 3;
        uint8_t pal1 = (tableNESPalette[c] & 0xe0)
            | ((tableNESPalette[c + 1] >> 3) & 0x1c)
            | (tableNESPalette[c + 2] >> 6);
        uint8_t pal2 = (tableNESPalette[c + 2] >> 5) & 1;
        ZXN_WRITE_REG(0x44, pal1);
        ZXN_WRITE_REG(0x44, pal2);
        // DEBUG_PRINTF("Color%x %x (%x %x %x) (%x %x)\n",
        //     i, palette[i], tableNESPalette[c], tableNESPalette[c + 1],
        //     tableNESPalette[c + 2], pal1, pal2);
    }
    // update picture
    uint8_t *screen = (uint8_t*)TILEMAP_BASE + 4 * LINE_BYTES;
    const uint8_t gap = 4 * 2;
    for (i = 0 ; i < 16 ; ++i)
    {
        screen += gap;
        for (j = 4 ; j < LINE_WIDTH - 4 ; ++j)
        {
            uint8_t x = j - 2;
            uint8_t y = i;
            uint8_t attr = (attributes[((y << 2) & 0x30) | ((x >> 2) & 0xF)]
                >> (((y & 2) << 1) | (x & 2))) & 0x3;
            *screen++ = nametable[i][j - 2];
            *screen++ = attr << 4;
        }
        screen += gap;
    }
    //DEBUG_HALT;
}
