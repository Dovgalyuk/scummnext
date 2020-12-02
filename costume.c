#include <arch/zxn.h>
#include <arch/zxn/esxdos.h>
#include "costume.h"
#include "resource.h"
#include "graphics.h"
#include "room.h"
#include "debug.h"

static uint8_t decodeNESCostume(Actor *act, uint8_t nextSprite)
{
    // TODO: sprite and animation allocation
    Animation *animation = &act->anim;
    // costume ID -> v1MMNESLookup[] -> desc -> lens & offs -> data -> Gfx & pal
    // _baseptr = _vm->getResourceAddress(rtCostume, id);
    // _dataOffsets = _baseptr + 2;
    HROOM src = seekResource(costumes + act->costume);

	// int anim;

    uint16_t size = readWord(src);
    // read data offsets from src

    // limb = 0 for v2 scumm
	// anim = 4 * cost.frame[limb] + newDirToOldDir(a->getFacing());
	// frameNum = cost.curpos[limb];
	// frame = src[src[2 * anim] + frameNum];

    // _numAnim = 0x17 => numframes = 6
    uint8_t anim = act->frame * 4; // + dir (0-3)
    esx_f_seek(src, 2 * anim, ESX_SEEK_FWD);
    uint8_t begin = readByte(src);
    uint8_t end = readByte(src);
    DEBUG_PRINTF("Decode animation of %u frames from %u\n", end, begin);
    DEBUG_ASSERT(end <= MAX_FRAMES, "decodeNESCostume");
    esx_f_seek(src, begin - 2 * anim - 1, ESX_SEEK_FWD);

    // read all frames
    animation->frames = end;
    uint8_t f;
    for (f = 0 ; f < end ; ++f)
    {
        uint8_t frame = readByte(src);

        uint16_t offset = READ_LE_UINT16(costdesc + v1MMNESLookup[act->costume] * 2 + 2);
        uint8_t numSprites = costlens[offset + frame + 2] + 1;
        HROOM sprdata = seekResource(&costumes[costdata_id]);
        // offset is the beginning
        // in scummvm data is decoded in backwards direction, from the end
        uint16_t sprOffs = READ_LE_UINT16(costoffs + 2 * (offset + frame) + 2) + 2;
        DEBUG_PRINTF("decode frame=%u numspr=%u\n", frame, numSprites);
        esx_f_seek(sprdata, sprOffs, ESX_SEEK_FWD);

        // bool flipped = (newDirToOldDir(a->getFacing()) == 1);
        // int left = 239, right = 0, top = 239, bottom = 0;
        // byte *maskBuf = _vm->getMaskBuffer(0, 0, 1);

        uint8_t anchor = nextSprite;
        animation->anchors[f] = nextSprite;

        // setup anchor and relative sprite attributes and patterns

        for (uint8_t spr = 0 ; spr < numSprites ; spr++)
        {
            uint8_t d0 = readByte(sprdata);
            uint8_t d1 = readByte(sprdata);
            uint8_t d2 = readByte(sprdata);
            uint8_t mask;
            uint8_t sprpal = 0;
            int8_t y, x;

            mask = (d0 & 0x80) ? 0x01 : 0x80;
        // 	if (flipped) {
        // 		mask = (mask == 0x80) ? 0x01 : 0x80;
        // 		x = -x;
        // 	}

            y = d0 << 1;
            y >>= 1;

            uint8_t tile = d1;

            sprpal = (d2 & 0x03) << 2;
            x = d2;
            x >>= 2;

            // setup tile
            graphics_loadSpritePattern(nextSprite, tile, mask, sprpal);

            // send attributes
            IO_SPRITE_SLOT = nextSprite;
            if (nextSprite == anchor)
            {
                animation->ax = x;
                animation->ay = y;
            }

            x = x - animation->ax;
            y = y - animation->ay;
            IO_SPRITE_ATTRIBUTE = x;
            IO_SPRITE_ATTRIBUTE = y;

            if (nextSprite == anchor)
            {
                IO_SPRITE_ATTRIBUTE = x < 0 ? 1 : 0;
                // invisible yet
                IO_SPRITE_ATTRIBUTE = 0x40 | (nextSprite & 0x3f);
                // anchor 4-bit sprite
                IO_SPRITE_ATTRIBUTE = 0x80;
            }
            else
            {
                IO_SPRITE_ATTRIBUTE = 0;
                IO_SPRITE_ATTRIBUTE = 0xc0 | (nextSprite & 0x3f);
                // relative sprite
                IO_SPRITE_ATTRIBUTE = 0x40;
            }

            DEBUG_PRINTF("Sprite %d tile=%d x=%d y=%d\n", nextSprite, tile, x, y);

            ++nextSprite;
        }
        closeRoom(sprdata);
    }

	// DEBUG_PRINTF("Decode costume %u size=%u\n", costume, size);
    // for (uint8_t i = 0 ; i < 16 ; ++i)
    //     DEBUG_PRINTF("%x ", readWord(r));
    // DEBUG_PUTS("\n");

    closeRoom(src);

    return nextSprite;
}

void costume_updateAll(void)
{
    // uint8_t i;
    // for (i = 0 ; i < 64 ; ++i)
    // {
    //     graphics_loadSpritePattern(i, i, 0x80, 0);
    //     // send attributes
    //     IO_SPRITE_SLOT = i;
    //     IO_SPRITE_ATTRIBUTE = (i & 0xf) << 4; // x
    //     IO_SPRITE_ATTRIBUTE = (i & 0xf0) + 0x20; // y
    //     IO_SPRITE_ATTRIBUTE = 0;
    //     IO_SPRITE_ATTRIBUTE = 0xc0 | (i & 0x3f);
    //     IO_SPRITE_ATTRIBUTE = 0x80 | (i & 0x40); // anchor 4-bit sprite
    // }
    // DEBUG_HALT;

    // // we always need a sprite for the cursor
    graphics_loadSpritePattern(SPRITE_CURSOR, 0xfe, 0x80, 0);

    // decode costume sprites
    uint8_t i;
    uint8_t nextSprite = 1;
    for (i = 0 ; i < ACTOR_COUNT ; ++i)
    {
        if (actors[i].room == currentRoom)
        {
            DEBUG_ASSERT(nextSprite <= 63, "costume_updateAll");
            nextSprite = decodeNESCostume(&actors[i], nextSprite);
        }
    }

    // clean unused sprites
    while (nextSprite <= 63)
    {
        IO_SPRITE_SLOT = nextSprite;
        IO_SPRITE_ATTRIBUTE = 0;
        IO_SPRITE_ATTRIBUTE = 0;
        IO_SPRITE_ATTRIBUTE = 0;
        IO_SPRITE_ATTRIBUTE = 0;
        ++nextSprite;
    }
}
