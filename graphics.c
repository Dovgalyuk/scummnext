#include <arch/zxn.h>
#include <arch/zxn/esxdos.h>
#include "graphics.h"
#include "resource.h"
#include "camera.h"
#include "room.h"
#include "debug.h"
#include "verbs.h"
#include "sprites.h"
#include "string.h"
#include "script.h"
#include "helper.h"

#define PAL_SPRITES 0x20
#define PAL_TILES   0x30

#define GRAPH_PAGE 3
//#define SCREEN_PAGE 11
extern uint8_t nametable[16][64];
extern uint8_t attributes[64];
extern uint8_t translationTable[];

// costume set
uint8_t costdesc[51];
uint8_t costlens[279];
uint8_t costoffs[556];
// static uint8_t costdata[11234];
uint8_t costdata_id;

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

__sfr __at 0xfe IO_FE;

//#define TESTING 1

void initGraphics(void)
{
    int i;

    // black border
    IO_FE = 0;
    // disable layer2
    //ZXN_WRITE_REG(0x69, 0x00);
    // enable tilemap with attributes
    ZXN_WRITE_REG(0x6B, 0x81);
    ZXN_WRITE_REG(0x6C, 0);
    // tilemap transparency index
    //ZXN_WRITE_REG(0x4C, 0);
    // sprite transparency index
    ZXN_WRITE_REG(0x4B, 0);
    // disable ULA
    ZXN_WRITE_REG(0x68, 0x80);


    // tilemap base address is 0x6000
    ZXN_WRITE_REG(0x6E, TILEMAP_BASE >> 8);
    // tile base address is 0x4000
    ZXN_WRITE_REG(0x6F, TILE_BASE >> 8);

    // // switch on ULANext
    // ZXN_WRITE_REG(0x43 = 1;

    // enable sprites and sprites over border
    ZXN_WRITE_REG(0x15, 0x23);
    // set sprites clipping window 320x256
    ZXN_WRITE_REG(0x19, 0);
    ZXN_WRITE_REG(0x19, 159);
    ZXN_WRITE_REG(0x19, 0);
    ZXN_WRITE_REG(0x19, 255);

#ifdef TESTING
    // palette
    ZXN_WRITE_REG(0x43, 0x30);
    for (i = 0 ; i < 16 ; ++i)
    {
        uint8_t c = i & 7;
        ZXN_WRITE_REG(0x40, i);
        ZXN_WRITE_REG(0x44, (c << 5) | (c << 2) | (c >> 1));
        ZXN_WRITE_REG(0x44, 0);
    }

                      uint8_t *p;

    // enable layer2
    // ZXN_WRITE_REG(0xLAYER_2_CONFIG = 2 | 1;

    // uint8_t *p = 0;
    // for (i = 0 ; i < 0x4000 ; ++i)
    //     *p++ = i;

    // // DEBUG_PUTS("Setting tiles\n");

    char j;
    PUSH_PAGE(2, 10);
    p = (uint8_t*)TILE_BASE;
    for (i = 0 ; i < 256 * 32 ; ++i)
        //for (j = 0 ; j < 32 ; ++j)
            *p++ = i;
    POP_PAGE(2);

    // // DEBUG_PUTS("Setting tilemap\n");

    PUSH_PAGE(3, 11);
    p = (uint8_t*)TILEMAP_BASE + LINE_BYTES * 26;
    for (i = 0 ; i < 256 ; ++i)
    {
        *p++ = i;
        *p++ = 0;//i << 4;
    }
    POP_PAGE(3);
#endif

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


static void updatePalette(HROOM r, uint8_t id)
{
    uint8_t i, c;
    ZXN_WRITE_REG(0x43, id);
    for (i = 0 ; i < 16 ; ++i)
    {
		c = readByte(r);
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
        {
            ZXN_WRITE_REG(0x40, i);
        }
        else
        {
            ZXN_WRITE_REG(0x40, (i & 3) | ((i & 0xc) << 2));
        }
        ZXN_WRITE_REG(0x44, tableNESPalette[c]);
        ZXN_WRITE_REG(0x44, tableNESPalette[c + 1]);
    }

    // dark palette for the sprites
    if (id == PAL_SPRITES)
    {
    	static const uint8_t darkpalette[16] = {
            0x00,0x00,0x2D,0x3D,0x00,0x00,0x2D,0x3D,
            0x00,0x00,0x2D,0x3D,0x00,0x00,0x2D,0x3D
        };
        const uint8_t *d = darkpalette;
        for (i = 16 ; i < 32 ; ++i)
        {
            c = *d;
            ++d;
            if (c == 0x1D)	// HACK - switch around colors 0x00 and 0x1D
                c = 0;		// so we don't need a zillion extra checks
            else if (c == 0)// for determining the proper background color
                c = 0x1D;
            c = c * 2;
            ZXN_WRITE_REG(0x40, i);
            ZXN_WRITE_REG(0x44, tableNESPalette[c]);
            ZXN_WRITE_REG(0x44, tableNESPalette[c + 1]);
        }
    }
}

void decodeRoomBackground(HROOM r)
{
    PUSH_PAGE(2, GRAPH_PAGE);

    esx_f_seek(r, 0xa, ESX_SEEK_SET);
    uint16_t gdata = readWord(r);
    // 17 = tileset_id + palette
    esx_f_seek(r, gdata + 17, ESX_SEEK_SET);

    uint8_t i, j;
    uint16_t n;
    uint8_t data = readByte(r);
    //DEBUG_PUTS("nametable:\n");
    for (i = 0 ; i < 16 ; ++i)
    {
        nametable[i][0] = nametable[i][1] = 0;
		n = 0;
		while (n < roomWidth)
        {
            uint8_t next = readByte(r);
			for (j = 0 ; j < (data & 0x7F) ; j++)
            {
                //if (i == 0) DEBUG_PRINTF("%x ", next);
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
    //DEBUG_PUTS("\n");
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

    POP_PAGE(2);
}

void decodeNESGfx(HROOM r)
{
    // uint16_t width;
    //esx_f_seek(r, 0x4, ESX_SEEK_SET);
    // width = readWord(r);
    esx_f_seek(r, 0xa, ESX_SEEK_SET);
    uint16_t gdata = readWord(r);
    esx_f_seek(r, gdata, ESX_SEEK_SET);
    uint8_t tileset = readByte(r);
    //DEBUG_PRINTF("decoding gdata %u tileset %u\n", gdata, tileset);
    decodeTiles(tileset);
    // decode palette
    updatePalette(r, PAL_TILES);
    // decode name table
    decodeRoomBackground(r);
 }

static const int v1MMNEScostTables[2][6] = {
	/* desc lens offs data  gfx  pal */
	{  25,  27,  29,  31,   33,  35},
	{  26,  28,  30,  32,   34,  36}
};

void graphics_loadCostumeSet(uint8_t n)
{
    HROOM r;
    //DEBUG_PRINTF("Load costume set %u\n", n);
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
    decodeSpriteTiles(v1MMNEScostTables[n][4]);

    // palette for sprites
    r = seekResource(&costumes[v1MMNEScostTables[n][5]]);
    // skip size
    readWord(r);
    updatePalette(r, PAL_SPRITES);
    closeRoom(r);
}

static void clearLine(uint8_t y)
{
    uint8_t *screen = (uint8_t*)TILEMAP_BASE + LINE_BYTES * y;
    //uint8_t *screen = (uint8_t*)0 + LINE_BYTES * y;
    uint8_t *end = screen + LINE_BYTES;
    while (screen != end)
    {
        // hardcoded black tile
        *screen++ = 0;
        // hardcoded background color offset
        *screen++ = 3 << 4;
    }
}

static void clearTalkArea(void)
{
    uint8_t i;
    for (i = 0 ; i < SCREEN_TOP ; ++i)
        clearLine(i);
}

void graphics_clearInventory(void)
{
    //PUSH_PAGE(0, SCREEN_PAGE);
    uint8_t i;
    for (i = 0 ; i < INV_ROWS ; ++i)
        clearLine(INV_TOP + i);
    //POP_PAGE(0);
}


void graphics_printAtXY(const uint8_t *s, uint8_t x, uint8_t y, uint8_t left, uint8_t color, uint8_t len)
{
    //PUSH_PAGE(0, SCREEN_PAGE);
    PUSH_PAGE(2, GRAPH_PAGE);

    uint8_t *screen = (uint8_t*)TILEMAP_BASE + x * TILE_BYTES + y * LINE_BYTES;
    //uint8_t *screen = (uint8_t*)0 + x * TILE_BYTES + y * LINE_BYTES;
    uint8_t curpos = 0;
    while (*s)
    {
        uint8_t c = *s++;
        if (curpos >= len)
            continue;
        if (c == '@')
        {
            continue;
        }
        else if (c == ' ')
        {
            // need special space color and symbol
            *screen++ = 0;
            *screen++ = 3 << 4;
            ++x;
            continue;
        }
        else if (c == 1)
        {
            curpos = 0;
            // newline
            if (x != 0)
            {
                screen += 2 * left * TILE_BYTES + (LINE_WIDTH - left - x) * TILE_BYTES;
                x = 0;
            }
            continue;
        }
        else if (c == 3)
        {
            // next message, should not be passed here
        }
        else if (c == 4)
        {
            // don't know what code is it, skip
            continue;
        }
        if (c < 0x20 || c >= 0x80)
        {
            DEBUG_PRINTF("\nSpecial code %u\n", c);
            DEBUG_HALT;
        }
        //DEBUG_PRINTF("char %u pattern %u\n", *s, translationTable[*s]);
        *screen++ = translationTable[c - ' '];
        *screen++ = color << 4;
        ++x;
        // if (x >= LINE_WIDTH - gap)
        // {
        //     x = 0;
        //     screen += 2 * gap * TILE_BYTES;
        // }
    }

    POP_PAGE(2);
    //POP_PAGE(0);
}

void graphics_print(const char *s, uint8_t c)
{
    //PUSH_PAGE(0, SCREEN_PAGE);
    clearTalkArea();
    // TODO: correct color
    c = 3;
    graphics_printAtXY(s, TEXT_GAP, 0, TEXT_GAP, c, SCREEN_WIDTH);
    //POP_PAGE(0);
}

void graphics_printSentence(const char *s)
{
    //PUSH_PAGE(0, SCREEN_PAGE);
    clearLine(SCREEN_HEIGHT + SCREEN_TOP);
    // color offset 3 to match transparent color
    graphics_printAtXY(s, TEXT_GAP, SCREEN_HEIGHT + SCREEN_TOP, TEXT_GAP, 3, SCREEN_WIDTH);
    //POP_PAGE(0);
}

void graphics_drawVerb(VerbSlot *v)
{
    // color offset 3 to match transparent color
    graphics_printAtXY(v->name, LINE_GAP + v->x, v->y, LINE_GAP, 3, SCREEN_WIDTH);
}

void graphics_drawInventory(uint8_t slot, const char *s)
{
    uint8_t x = 0;
    // only 2 cols supported
    if (slot % INV_COLS)
        x = INV_SLOT_WIDTH + INV_ARR_W;
    // color offset 3 to match transparent color
    graphics_printAtXY(s, INV_GAP + x,
        INV_TOP + slot / INV_COLS, INV_GAP, 3, INV_SLOT_WIDTH);
}

void graphics_clearScreen(void)
{
    uint8_t i;

    // clear sprites first
    for (i = 0 ; i <= 127 ; ++i)
    {
        IO_SPRITE_SLOT = i;
        IO_SPRITE_ATTRIBUTE = 0;
        IO_SPRITE_ATTRIBUTE = 0;
        IO_SPRITE_ATTRIBUTE = 0;
        IO_SPRITE_ATTRIBUTE = 0;
    }

    //PUSH_PAGE(0, SCREEN_PAGE);
    // clear tiles after
    for (i = 0 ; i < SCREEN_TOP + SCREEN_HEIGHT ; ++i)
        clearLine(i);
    //POP_PAGE(0);
}

void graphics_updateScreen(void)
{
    //PUSH_PAGE(0, SCREEN_PAGE);
    PUSH_PAGE(2, GRAPH_PAGE);

    //DEBUG_PRINTF("Draw nametable %x\n", nametable[15][2]);

    uint8_t i, j, x, y;
    uint16_t xx;
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
    //uint8_t *screen = (uint8_t*)0 + SCREEN_TOP * LINE_BYTES;

    uint8_t gap = LINE_GAP;
    uint8_t offs;
    uint8_t width = SCREEN_WIDTH;
    if (roomWidth < SCREEN_WIDTH)
    {
        gap += (SCREEN_WIDTH - roomWidth) / 2;
        width = roomWidth;
    }
    offs = camera_getVirtScreenX();

    uint8_t light = scummVars[VAR_CURRENT_LIGHTS] & LIGHTMODE_actor_use_base_palette;

    uint8_t bytegap = gap * 2;
    //DEBUG_PRINTF("cameraX=%u roomWidth=%u offs=%u gap=%u\n", cameraX, roomWidth, offs, gap);
    // DEBUG_PRINTF("Screen bytes %x %x\n",
    //      nametable[0][2],
    //      nametable[0][offs + 2]);
    for (i = 0 ; i < SCREEN_HEIGHT ; ++i)
    {
        screen += bytegap;
        for (j = 0 ; j < width ; ++j)
        {
            x = j + offs + 2;
            y = i;
            uint8_t attr = (attributes[((y << 2) & 0x30) | ((x >> 2) & 0xF)]
                >> (((y & 2) << 1) | (x & 2))) & 0x3;
            /* 2 empty cells at the beginning */
            if (light)
            {
                //if (i == 0) DEBUG_PRINTF("%x ", nametable[i][offs + j + 2]);
                *screen = nametable[i][offs + j + 2];
                ++screen;
                *screen = attr << 4;
                ++screen;
            }
            else
            {
                *screen = 0;
                ++screen;
                *screen = 0;
                ++screen;
            }
        }
        screen += bytegap;
    }
    //DEBUG_PUTS("\n");

    POP_PAGE(2);
    //POP_PAGE(0);

    actors_draw(offs, gap);
}

void graphics_drawObject(uint16_t bimoffs, uint8_t ox, uint8_t oy, uint8_t w, uint8_t h)
{
    if (!bimoffs)
        return;

    //DEBUG_PRINTF("--- decode object ox=%u oy=%u w=%u h=%u\n", ox, oy, w, h);

    HROOM r = openRoom(currentRoom);
    esx_f_seek(r, bimoffs, ESX_SEEK_SET);

    PUSH_PAGE(2, GRAPH_PAGE);
    uint8_t len = readByte(r);
    for (uint8_t y = 0 ; y < h ; ++y)
    {
        uint8_t x = 0;
        while (x < w)
        {
            uint8_t next = readByte(r);
            for (uint8_t i = 0 ; i < (len & 0x7f) ; ++i)
            {
                // DEBUG_PRINTF("%x ", next);
                nametable[oy + y][x + ox + 2] = next;
                if (len & 0x80)
                    next = readByte(r);
                ++x;
            }
            if (!(len & 0x80))
                next = readByte(r);
            len = next;
        }
        // DEBUG_PUTS("\n");
    }

    // decode attribute update data
    uint8_t y = h / 2;
    uint8_t ay = oy;
	while (y)
    {
		uint8_t ax = ox + 2;
		uint8_t x = 0;
		uint8_t adata = 0;
		while (x < (w >> 1))
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

    POP_PAGE(2);
}

uint8_t graphics_findVirtScreen(uint8_t y)
{
    if (y <= SCREEN_TOP)
        return kTextVirtScreen;
    if (y <= SCREEN_TOP + SCREEN_HEIGHT)
        return kMainVirtScreen;
    return kVerbVirtScreen;
}
