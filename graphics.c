#include <arch/zxn.h>
#include <arch/zxn/esxdos.h>
#include "graphics.h"
#include "resource.h"
#include "camera.h"
#include "room.h"
#include "debug.h"
#include "verbs.h"

#define TILEMAP_BASE 0x6000
#define TILE_BASE 0x4000
#define TILE_BYTES 2
#define LINE_BYTES (LINE_WIDTH * TILE_BYTES)

#define PAL_SPRITES 0x20
#define PAL_TILES   0x30

static uint8_t baseTilesCount;
static uint16_t tilesCount;
static uint8_t translationTable[256];

static uint8_t nametable[16][64];
static uint8_t attributes[64];
// costume set
uint8_t costdesc[51];
uint8_t costlens[279];
uint8_t costoffs[556];
// static uint8_t costdata[11234];
uint8_t costdata_id;
//uint8_t spriteTiles[4096];
extern uint8_t spriteTiles[];
// this is temporary
//static uint8_t tileBuf[4096];
//#define tileBuf ((uint8_t*)0x3000)
extern uint8_t tileBuf[];

#define RGB2NEXT(r, g, b) (uint8_t)(((r) & 0xe0) | (((g) >> 3) & 0x1c) | (((b) >> 6) & 0x3)), \
                          (((b) >> 5) & 1)
// #define RGB2NEXT(r, g, b) r, g, b


static const uint8_t tableNESPalette[] = {
    /*    0x1D     */
    RGB2NEXT(0x24, 0x24, 0x24), RGB2NEXT(0x00, 0x24, 0x92),
    RGB2NEXT(0x00, 0x00, 0xDB), RGB2NEXT(0x6D, 0x49, 0xDB),
    RGB2NEXT(0x92, 0x00, 0x6D), RGB2NEXT(0xB6, 0x00, 0x6D), 	
    RGB2NEXT(0xB6, 0x24, 0x00), RGB2NEXT(0x92, 0x49, 0x00),
    RGB2NEXT(0x6D, 0x49, 0x00), RGB2NEXT(0x24, 0x49, 0x00),
    RGB2NEXT(0x00, 0x6D, 0x24), RGB2NEXT(0x00, 0x92, 0x00),
    RGB2NEXT(0x00, 0x49, 0x49), RGB2NEXT(0x00, 0x00, 0x00),
    RGB2NEXT(0x00, 0x00, 0x00), RGB2NEXT(0x00, 0x00, 0x00),

    RGB2NEXT(0xB6, 0xB6, 0xB6), RGB2NEXT(0x00, 0x6D, 0xDB), 
    RGB2NEXT(0x00, 0x49, 0xFF), RGB2NEXT(0x92, 0x00, 0xFF),
    RGB2NEXT(0xB6, 0x00, 0xFF), RGB2NEXT(0xFF, 0x00, 0x92), 
    RGB2NEXT(0xFF, 0x00, 0x00), RGB2NEXT(0xDB, 0x6D, 0x00),
    RGB2NEXT(0x92, 0x6D, 0x00), RGB2NEXT(0x24, 0x92, 0x00), 
    RGB2NEXT(0x00, 0x92, 0x00), RGB2NEXT(0x00, 0xB6, 0x6D),
                                /*    0x00     */
    RGB2NEXT(0x00, 0x92, 0x92), RGB2NEXT(0x6D, 0x6D, 0x6D), 
    RGB2NEXT(0x00, 0x00, 0x00), RGB2NEXT(0x00, 0x00, 0x00),

    RGB2NEXT(0xFF, 0xFF, 0xFF), RGB2NEXT(0x6D, 0xB6, 0xFF),
    RGB2NEXT(0x92, 0x92, 0xFF), RGB2NEXT(0xDB, 0x6D, 0xFF),
    RGB2NEXT(0xFF, 0x00, 0xFF), RGB2NEXT(0xFF, 0x6D, 0xFF),
    RGB2NEXT(0xFF, 0x92, 0x00), RGB2NEXT(0xFF, 0xB6, 0x00),
    RGB2NEXT(0xDB, 0xDB, 0x00), RGB2NEXT(0x6D, 0xDB, 0x00),
    RGB2NEXT(0x00, 0xFF, 0x00), RGB2NEXT(0x49, 0xFF, 0xDB),
    RGB2NEXT(0x00, 0xFF, 0xFF), RGB2NEXT(0x49, 0x49, 0x49),
    RGB2NEXT(0x00, 0x00, 0x00), RGB2NEXT(0x00, 0x00, 0x00),

    RGB2NEXT(0xFF, 0xFF, 0xFF), RGB2NEXT(0xB6, 0xDB, 0xFF),
    RGB2NEXT(0xDB, 0xB6, 0xFF), RGB2NEXT(0xFF, 0xB6, 0xFF),
    RGB2NEXT(0xFF, 0x92, 0xFF), RGB2NEXT(0xFF, 0xB6, 0xB6),
    RGB2NEXT(0xFF, 0xDB, 0x92), RGB2NEXT(0xFF, 0xFF, 0x49),
    RGB2NEXT(0xFF, 0xFF, 0x6D), RGB2NEXT(0xB6, 0xFF, 0x49),
    RGB2NEXT(0x92, 0xFF, 0x6D), RGB2NEXT(0x49, 0xFF, 0xDB),
    RGB2NEXT(0x92, 0xDB, 0xFF), RGB2NEXT(0x92, 0x92, 0x92),
    RGB2NEXT(0x00, 0x00, 0x00), RGB2NEXT(0x00, 0x00, 0x00)
};

const uint8_t v1MMNESLookup[25] = {
	0x00, 0x03, 0x01, 0x06, 0x08,
	0x02, 0x00, 0x07, 0x0C, 0x04,
	0x09, 0x0A, 0x12, 0x0B, 0x14,
	0x0D, 0x11, 0x0F, 0x0E, 0x10,
	0x17, 0x00, 0x01, 0x05, 0x16
};

void initGraphics(void)
{
    int i;

    // disable layer2
    //ZXN_WRITE_REG(0x69, 0x00);
    // enable tilemap with attributes
    ZXN_WRITE_REG(0x6B, 0x81);
    ZXN_WRITE_REG(0x6C, 0);
    // transparency index
    //ZXN_WRITE_REG(0x4C, 0);
    // sprite transparency index
    ZXN_WRITE_REG(0x4B, 0);
    // disable ULA
    ZXN_WRITE_REG(0x68, 0x80);


    // tilemap base address is 0x6000
    ZXN_WRITE_REG(0x6E, TILEMAP_BASE >> 8);
    // tile base address is 0x6500
    ZXN_WRITE_REG(0x6F, TILE_BASE >> 8);

    // // switch on ULANext
    // ZXN_WRITE_REG(0x43 = 1;
    // // disable ULA output
    // ZXN_WRITE_REG(0x68 = 0x80;
    
    // enable sprites and sprites over border
    ZXN_WRITE_REG(0x15, 0x23);
    // set sprites clipping window 320x256
    ZXN_WRITE_REG(0x19, 0);
    ZXN_WRITE_REG(0x19, 159);
    ZXN_WRITE_REG(0x19, 0);
    ZXN_WRITE_REG(0x19, 255);
    // setup sprites first palette
    // ZXN_WRITE_REG(0x43, 0x20);
    // for (i = 0 ; i < sizeof(tableNESPalette) / 2 ; ++i)
    // {
    //     ZXN_WRITE_REG(0x40, i);
    //     ZXN_WRITE_REG(0x44, tableNESPalette[2 * i]);
    //     ZXN_WRITE_REG(0x44, tableNESPalette[2 * i + 1]);
    // }
    
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

    uint8_t *p = (uint8_t*)TILEMAP_BASE + LINE_BYTES * 26;
    for (i = 0 ; i < 256 ; ++i)
    {
        *p++ = i;
        *p++ = 0;
    }

    // reset all sprite attributes
    for (i = 0 ; i < 128 ; ++i)
    {
        IO_SPRITE_SLOT = i;
        IO_SPRITE_ATTRIBUTE = 0;
        IO_SPRITE_ATTRIBUTE = 0;
        IO_SPRITE_ATTRIBUTE = 0;
        IO_SPRITE_ATTRIBUTE = 0;
    }

    // // DEBUG_PUTS("End setup\n");
}

static uint16_t decodeNESTileData(uint8_t *buf, HROOM f, uint16_t len)
{
    uint8_t *dst = buf;
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

    return dst - buf;
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

static uint16_t decodeNESTiles(uint8_t *buf, uint8_t set)
{
    HROOM f = seekResource(&costumes[set]);
    uint16_t len = readWord(f);
    uint16_t count = readByte(f);
    uint16_t bytes = decodeNESTileData(buf, f, len - 1);
    closeRoom(f);
    if (count == 0)
        count = bytes / 16;
    DEBUG_PRINTF("NES tiles %u %u/%u\n", count, len, bytes);
    return count;
}

static void setTiles(uint16_t count)
{
    uint8_t *tiles = (uint8_t*)TILE_BASE + baseTilesCount * 32;
    //uint8_t *screen = (uint8_t*)TILEMAP_BASE;
    if (!baseTilesCount)
        baseTilesCount = count;
    tilesCount = baseTilesCount + count;
    for (uint16_t c = 0 ; c < count ; ++c)
    {
        // DEBUG_PRINTF("Tile %u: ", c);
        for (uint8_t i = 0 ; i < 8 ; ++i)
        {
            uint8_t c0 = tileBuf[c * 16 + i];
            uint8_t c1 = tileBuf[c * 16 + i + 8];
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

// static void setSprites(uint16_t count)
// {
//     DEBUG_PRINTF("Setup %u sprites\n", count);
//     uint8_t i;
//     // converts 8x8 NES sprites to 16x16 ZX Next
//     if (count > 128)
//         count = 128;
//     IO_SPRITE_SLOT = 0;
//     for (uint16_t c = 0 ; c < count ; ++c)
//     {
//         // send pattern
//         for (i = 0 ; i < 8 ; ++i)
//         {
//             uint8_t c0 = tileBuf[c * 16 + i];
//             uint8_t c1 = tileBuf[c * 16 + i + 8];
//             for (uint8_t j = 0 ; j < 8 ; ++j)
//             {
//                 uint8_t col1 = ((c0 >> (7 - j)) & 1) + (((c1 >> (7 - j)) & 1) << 1);
//                 ++j;
//                 uint8_t col2 = ((c0 >> (7 - j)) & 1) + (((c1 >> (7 - j)) & 1) << 1);
//                 uint8_t b = (col1 << 4) | col2;
//                 IO_SPRITE_PATTERN = b;
//             }
//             IO_SPRITE_PATTERN = 0x0;
//             IO_SPRITE_PATTERN = 0x0;
//             IO_SPRITE_PATTERN = 0x0;
//             IO_SPRITE_PATTERN = 0x0;
//         }
//         for (i = 0 ; i < 64 ; ++i)
//             IO_SPRITE_PATTERN = 0x0;
//     }

//     // for (i = 0 ; i < 128 ; ++i)
//     // {
//     //     // send attributes
//     //     IO_SPRITE_SLOT = i;
//     //     IO_SPRITE_ATTRIBUTE = (i & 0xf) * 17; // x
//     //     IO_SPRITE_ATTRIBUTE = (i & 0xf0); // y
//     //     IO_SPRITE_ATTRIBUTE = 0;
//     //     IO_SPRITE_ATTRIBUTE = 0xc0 | (i & 0x3f);
//     //     IO_SPRITE_ATTRIBUTE = 0x80 | (i & 0x40); // anchor 4-bit sprite
//     // }

//     // sprite transparency register
//     ZXN_WRITE_REG(0x4b, 0);
// }

void decodeNESBaseTiles(void)
{
    uint16_t count = decodeNESTiles(tileBuf, 37);
    setTiles(count);
}

static void updatePalette(HROOM r, uint8_t id)
{
    uint8_t i;
    ZXN_WRITE_REG(0x43, id);
    for (i = 0 ; i < 16 ; ++i)
    {
		uint8_t c = readByte(r);
        // TODO: this is for background tiles only
		// if (c == 0x0D)
		// 	c = 0x1D;
		if (c == 0x1D)	// HACK - switch around colors 0x00 and 0x1D
			c = 0;		// so we don't need a zillion extra checks
		else if (c == 0)// for determining the proper background color
			c = 0x1D;
        c = c * 2;
        // color(0-3) + palette offset(0-3)
        if (id == PAL_SPRITES)
            ZXN_WRITE_REG(0x40, i);
        else
            ZXN_WRITE_REG(0x40, (i & 3) | ((i & 0xc) << 2));
        ZXN_WRITE_REG(0x44, tableNESPalette[c]);
        ZXN_WRITE_REG(0x44, tableNESPalette[c + 1]);
    }
}

void decodeNESGfx(HROOM r)
{
    uint8_t i, j;
    uint16_t n;
    // uint16_t width;
    //esx_f_seek(r, 0x4, ESX_SEEK_SET);
    // width = readWord(r);
    esx_f_seek(r, 0xa, ESX_SEEK_SET);
    uint16_t gdata = readWord(r);
    esx_f_seek(r, gdata, ESX_SEEK_SET);
    uint8_t tileset = readByte(r);
    DEBUG_PRINTF("decoding gdata %u tileset %u\n", gdata, tileset);
    setTiles(decodeNESTiles(tileBuf, 37 + tileset));
    // decode palette
    updatePalette(r, PAL_TILES);
    // decode name table
    uint8_t data = readByte(r);
    for (i = 0 ; i < 16 ; ++i)
    {
        nametable[i][0] = nametable[i][1] = 0;
		n = 0;
		while (n < roomWidth)
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
		nametable[i][roomWidth + 2] = nametable[i][roomWidth + 3] = 0;
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
		if (!(n & 7) && (roomWidth == 0x1C))
			n += 8;
		if (!(data & 0x80))
			next = readByte(r);
        data = next;
	}
    // decode masktable
}

static const int v1MMNEScostTables[2][6] = {
	/* desc lens offs data  gfx  pal */
	{ 25,  27,  29,  31,  33,  35},
	{ 26,  28,  30,  32,  34,  36}
};

void graphics_loadCostumeSet(uint8_t n)
{
    HROOM r;
    DEBUG_PRINTF("Load costume set %u\n", n);
    r = seekResource(&costumes[v1MMNEScostTables[n][0]]);
    readResource(r, costdesc, sizeof(costdesc));
    closeRoom(r);
    r = seekResource(&costumes[v1MMNEScostTables[n][1]]);
    readResource(r, costlens, sizeof(costlens));
    closeRoom(r);
    r = seekResource(&costumes[v1MMNEScostTables[n][2]]);
    readResource(r, costoffs, sizeof(costoffs));
    closeRoom(r);

    // read this directly from the file
    costdata_id = v1MMNEScostTables[n][3];
    // r = seekResource(&costumes[v1MMNEScostTables[n][3]]);
    // readResource(r, costdata, sizeof(costdata));
    // closeRoom(r);

    // decode tiles, but do not copy them to ZXNext yet,
    // because 256 patterns won't fit
    decodeNESTiles(spriteTiles, v1MMNEScostTables[n][4]);
    // int i;
    // int count = 0;
    // for (i = 0 ; i < 256 ; ++i)
    // {
    //     uint8_t sum = 0;
    //     uint8_t j;
    //     uint8_t *p = &spriteTiles[i * 16];
    //     for (j = 0 ; j < 16 ; ++j)
    //         sum |= *p++;
    //     if (sum)
    //         ++count;
    // }
    // DEBUG_PRINTF("decoded %u non-empty sprite tiles\n", count);

    // palette for sprites
    r = seekResource(&costumes[v1MMNEScostTables[n][5]]);
    // skip size
    readWord(r);
    updatePalette(r, PAL_SPRITES);
    closeRoom(r);
}

static void decodeNESObject(Object *obj)
{
    HROOM r = openRoom(currentRoom);
    esx_f_seek(r, obj->OBIMoffset, ESX_SEEK_SET);

    uint8_t len = readByte(r);
    for (uint8_t y = 0 ; y < obj->height ; ++y)
    {
        uint8_t next = readByte(r);
        uint8_t x = 0;
        while (x < obj->width)
        {
            for (uint8_t i = 0 ; i < (len & 0x7f) ; ++i)
            {
                //DEBUG_PRINTF("%x ", next);
                nametable[obj->y + y][x + obj->x + 2] = next;
                if (len & 0x80)
                    next = readByte(r);
                ++x;
            }
            if (!(len & 0x80))
                next = readByte(r);
            len = next;
        }
        //DEBUG_PUTS("\n");
    }

	// decode attribute update data
	uint8_t y = obj->height / 2;
	uint8_t ay = obj->y;
	while (y)
    {
		uint8_t ax = obj->x + 2;
		uint8_t x = 0;
		uint8_t adata = 0;
		while (x < (obj->width >> 1))
        {
			if (!(x & 3))
				adata = readByte(r);
			uint8_t *dest = &attributes[((ay << 2) & 0x30) | ((ax >> 2) & 0xF)];

			uint8_t aand = 3;
			uint8_t aor = adata & 3;
			if (ay & 0x02)
            {
				aand <<= 4;
				aor <<= 4;
			}
			if (ax & 0x02)
            {
				aand <<= 2;
				aor <<= 2;
			}
			*dest = ((~aand) & *dest) | aor;

			adata >>= 2;
			ax += 2;
			x++;
		}
		ay += 2;
		y--;
	}

	// decode mask update data
	// if (!_NES.hasmask)
	// 	return;
	// int mx, mwidth;
	// int lmask, rmask;
	// mx = *ptr++;
	// mwidth = *ptr++;
	// lmask = *ptr++;
	// rmask = *ptr++;

	// for (y = 0; y < height; ++y) {
	// 	byte *dest = &_NES.masktableObj[y + ypos][mx];
	// 	*dest = (*dest & lmask) | *ptr++;
	// 	dest++;
	// 	for (x = 1; x < mwidth; x++) {
	// 		if (x + 1 == mwidth)
	// 			*dest = (*dest & rmask) | *ptr++;
	// 		else
	// 			*dest = *ptr++;
	// 		dest++;
	// 	}
	// }

    closeRoom(r);
}

void graphics_loadSpritePattern(uint8_t nextSprite, uint8_t tile, uint8_t mask, uint8_t sprpal)
{
    IO_SPRITE_SLOT = nextSprite;
    uint8_t i;
    //DEBUG_PRINTF("tile: %x\n", tile);
    uint8_t *t = &spriteTiles[tile * 16];
    for (i = 0 ; i < 8 ; ++i)
    {
        uint8_t c0 = t[i];
        uint8_t c1 = t[i + 8];
        // tile data is ok
        //DEBUG_PRINTF("tile line: %x %x\n", c0, c1);
        uint8_t c = 0;
        for (uint8_t j = 0 ; j < 8 ; ++j)
        {
            uint8_t cc = ((c0 & mask) ? 1 : 0) | ((c1 & mask) ? 2 : 0);
            // zero is transparent
            if (cc)
                cc |= sprpal;
            c = (c << 4) | cc;
            if (j & 1)
            {
                IO_SPRITE_PATTERN = c;
            }

            if (mask == 0x01) {
                c0 >>= 1;
                c1 >>= 1;
            } else {
                c0 <<= 1;
                c1 <<= 1;
            }
        }
        IO_SPRITE_PATTERN = 0x0;
        IO_SPRITE_PATTERN = 0x0;
        IO_SPRITE_PATTERN = 0x0;
        IO_SPRITE_PATTERN = 0x0;
    }
    for (i = 0 ; i < 64 ; ++i)
        IO_SPRITE_PATTERN = 0x0;
    // second pattern, unused yet
    for (i = 0 ; i < 128 ; ++i)
        IO_SPRITE_PATTERN = 0x0;
}

static void clearTalkArea(void)
{
    uint8_t *screen = (uint8_t*)TILEMAP_BASE;
    uint8_t *end = (uint8_t*)TILEMAP_BASE + SCREEN_TOP * LINE_BYTES;
    while (screen != end)
    {
        // hardcoded black tile
        *screen++ = 1;
        *screen++ = 0;
    }
}

static void printAtXY(const char *s, uint8_t x, uint8_t y)
{
    uint8_t *screen = (uint8_t*)TILEMAP_BASE + x * TILE_BYTES + y * LINE_BYTES;
    while (*s)
    {
        //DEBUG_PRINTF("char %u pattern %u\n", *s, translationTable[*s]);
        *screen++ = translationTable[*s++];
        *screen++ = 0;
    }
}

void graphics_print(const char *s)
{
    clearTalkArea();
    printAtXY(s, 0, 0);
}

void graphics_drawVerb(VerbSlot *v)
{
    printAtXY(v->name, LINE_GAP + v->x, v->y);
}

void graphics_updateScreen(void)
{
    uint8_t i, j;
    // update message area
    if (talkDelay)
    {
        if (!--talkDelay)
        {
            clearTalkArea();
        }
    }

    // update background picture
    uint8_t *screen = (uint8_t*)TILEMAP_BASE + SCREEN_TOP * LINE_BYTES;

    uint8_t gap = LINE_GAP;
    uint8_t offs;
    uint8_t width = SCREEN_WIDTH;
    if (roomWidth < SCREEN_WIDTH)
    {
        gap += (SCREEN_WIDTH - roomWidth) / 2;
        offs = 0;
        width = roomWidth;
    }
    else
    {
        uint8_t cam = cameraX;
        if (cam < SCREEN_WIDTH / 2)
            cam = SCREEN_WIDTH / 2;
        else if (cam > roomWidth - SCREEN_WIDTH / 2)
            cam = roomWidth - SCREEN_WIDTH / 2;
        offs = cam - SCREEN_WIDTH / 2;
    }

    // TODO: clear the border

    uint8_t bytegap = gap * 2;
    //DEBUG_PRINTF("cameraX=%u roomWidth=%u offs=%u gap=%u\n", cameraX, roomWidth, offs, gap);
    for (i = 0 ; i < 16 ; ++i)
    {
        screen += bytegap;
        for (j = 0 ; j < width ; ++j)
        {
            uint8_t x = j + offs + 2;
            uint8_t y = i;
            uint8_t attr = (attributes[((y << 2) & 0x30) | ((x >> 2) & 0xF)]
                >> (((y & 2) << 1) | (x & 2))) & 0x3;
            /* 2 empty cells at the beginning */
            *screen++ = nametable[i][offs + j + 2];
            *screen++ = attr << 4;
        }
        screen += bytegap;
    }

    // draw actors
    for (i = 0 ; i < ACTOR_COUNT ; ++i)
    {
        if (actors[i].room == currentRoom)
        {
            Animation *anim = &actors[i].anim;
            int16_t x = actors[i].x * V12_X_MULTIPLIER + anim->ax - offs * 8;
            uint8_t f;
            for (f = 0 ; f < anim->frames ; ++f)
            {
                uint8_t anchor = anim->anchors[f];
                IO_SPRITE_SLOT = anchor;
                if (f != anim->curpos || x < 0 || x > SCREEN_WIDTH * 8)
                {
                    IO_SPRITE_ATTRIBUTE = 0;
                    IO_SPRITE_ATTRIBUTE = 0;
                    IO_SPRITE_ATTRIBUTE = 0;
                    // invisible
                    IO_SPRITE_ATTRIBUTE = 0x40 | anchor;
                    IO_SPRITE_ATTRIBUTE = 0x80;
                    continue;
                }
                x += gap * 8;
                uint8_t y = actors[i].y * V12_Y_MULTIPLIER + anim->ay + SCREEN_TOP * 8;
                IO_SPRITE_ATTRIBUTE = x;
                IO_SPRITE_ATTRIBUTE = y;
                IO_SPRITE_ATTRIBUTE = (x >> 8) & 1;
                IO_SPRITE_ATTRIBUTE = 0xc0 | anchor;
                IO_SPRITE_ATTRIBUTE = 0x80;
                //DEBUG_PRINTF("Actor %u x=%u/%u y=%u/%u\n", i, actors[i].x, x, actors[i].y, y);
            }
            //graphics_drawActor(actors + i);
            //a->animateCostume();
        }
    }
}

void graphics_drawObject(Object *obj)
{
    if (!obj->OBIMoffset)
        return;
    // TODO: room?
    decodeNESObject(obj);
}
