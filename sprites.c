#include <arch/zxn.h>
#include "sprites.h"
#include "graphics.h"
#include "room.h"
#include "resource.h"
#include "debug.h"
#include "helper.h"

static uint8_t baseTilesCount;
//uint8_t spriteTiles[4096];
extern uint8_t spriteTiles[4096];
//#define tileBuf ((uint8_t*)0x3000)
extern uint8_t tileBuf[4096];

#define BUF_PAGE 2
#define TILEMAP_PAGE 10

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

static void setTiles(uint16_t count)
{
    PUSH_PAGE(0, TILEMAP_PAGE);
    uint8_t *tiles = (uint8_t*)0 + baseTilesCount * 32;
    //uint8_t *tiles = (uint8_t*)TILE_BASE + baseTilesCount * 32;
    if (!baseTilesCount)
        baseTilesCount = count;
    for (uint16_t c = 0 ; c < count ; ++c)
    {
        //if (c < 2)
        //DEBUG_PRINTF("Tile %u:\n", c);
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
                *tiles = b;
                ++tiles;
                // if (c == 1)
                // //DEBUG_PRINTF("%x%x", col1, col2);
                // DEBUG_PRINTF("%x %x ", tiles, b);
            }
            // DEBUG_PUTS("\n");
        }
        // if (c == 1)
        // DEBUG_PUTS("\n");
        //*screen++ = c;
    }
    POP_PAGE(0);
}

// Buffer should be in page >= 2
static uint16_t decodeNESTiles(uint8_t *buf, uint8_t set)
{
    HROOM f = seekResource(&costumes[set]);
    uint16_t len = readWord(f);
    uint16_t count = readByte(f);
    uint16_t bytes = decodeNESTileData(buf, f, len - 1);
    closeRoom(f);
    if (count == 0)
        count = bytes / 16;
    // DEBUG_PRINTF("NES tiles %u %u/%u\n", count, len, bytes);
    return count;
}

void decodeSpriteTiles(uint8_t set)
{
    PUSH_PAGE(2, BUF_PAGE);
    decodeNESTiles(spriteTiles, set);
    POP_PAGE(2);
}

void graphics_loadSpritePattern(uint8_t nextSprite, uint8_t tile, uint8_t mask, uint8_t sprpal)
{
    PUSH_PAGE(2, BUF_PAGE);

    uint8_t spr = (nextSprite & 0x3f) | ((nextSprite & 0x40) << 1);

    IO_SPRITE_SLOT = spr;
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
#ifdef HABR
            if (i == 0 || j == 0)
            {
                cc = 0x2;
            }
#endif
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
#ifdef HABR
        if (i == 0)
        {
            IO_SPRITE_PATTERN = 0x22;
            IO_SPRITE_PATTERN = 0x22;
            IO_SPRITE_PATTERN = 0x22;
            IO_SPRITE_PATTERN = 0x22;
        }
        else
        {
            IO_SPRITE_PATTERN = 0x0;
            IO_SPRITE_PATTERN = 0x0;
            IO_SPRITE_PATTERN = 0x0;
            IO_SPRITE_PATTERN = 0x02;
        }
#else
        IO_SPRITE_PATTERN = 0x0;
        IO_SPRITE_PATTERN = 0x0;
        IO_SPRITE_PATTERN = 0x0;
        IO_SPRITE_PATTERN = 0x0;
#endif
    }
    for (i = 0 ; i < 64 ; ++i)
    {
#ifdef HABR
        if (i >= 64 - 8)
            IO_SPRITE_PATTERN = 0x22;
        else if (i % 8 == 0)
            IO_SPRITE_PATTERN = 0x20;
        else if (i % 8 == 7)
            IO_SPRITE_PATTERN = 0x02;
        else
#endif
            IO_SPRITE_PATTERN = 0x0;
    }

    POP_PAGE(2);
}

void decodeTiles(uint8_t set)
{
    // DEBUG_PRINTF("Decoding tiles set %u\n", set);
    PUSH_PAGE(2, BUF_PAGE);
    setTiles(decodeNESTiles(tileBuf, 37 + set));
    POP_PAGE(2);
}
