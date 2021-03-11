#include <arch/zxn.h>
#include <arch/zxn/esxdos.h>
#include "costume.h"
#include "resource.h"
#include "graphics.h"
#include "room.h"
#include "sprites.h"
#include "debug.h"

extern uint8_t costume31[11234];
extern uint8_t costume32[140];

#define COST_PAGE0 44
#define COST_PAGE1 45

static uint8_t decodeNESCostume(Actor *act, uint8_t nextSprite)
{
    // TODO: sprite and animation allocation
    Animation *animation = &act->anim;
    // costume ID -> v1MMNESLookup[] -> desc -> lens & offs -> data -> Gfx & pal
    // _baseptr = _vm->getResourceAddress(rtCostume, id);
    // _dataOffsets = _baseptr + 2;
    //DEBUG_PRINTF("Decoding costume %d\n", costumes[act->costume]);
    HROOM src = seekResource(costumes + act->costume);

	// int anim;

    uint16_t size = readWord(src);
    //DEBUG_PRINTF("Costume %d size %d\n", act->costume, size);
    // read data offsets from src

    // limb = 0 for v2 scumm
	// anim = 4 * cost.frame[limb] + newDirToOldDir(a->getFacing());
	// frameNum = cost.curpos[limb];
	// frame = src[src[2 * anim] + frameNum];

    // _numAnim = 0x17 => numframes = 6
    uint8_t anim = act->frame * 4 + act->facing;
    esx_f_seek(src, 2 * anim, ESX_SEEK_FWD);
    uint8_t begin = readByte(src);
    uint8_t end = readByte(src);
    DEBUG_PRINTF("Decode animation %u/%u of %u frames from %u\n", anim, act->costume, end, begin);
    DEBUG_ASSERT(end <= MAX_FRAMES, "decodeNESCostume");
    esx_f_seek(src, begin - 2 * anim - 2, ESX_SEEK_FWD);

    // read all frames
    animation->frames = end;
    uint8_t f;
    uint8_t *sprdata = costdata_id == 31 ? costume31 : costume32;
    uint8_t flipped = act->facing == 1;
    for (f = 0 ; f < end ; ++f)
    {
        uint8_t frame = readByte(src);

        uint16_t offset = READ_LE_UINT16(costdesc + v1MMNESLookup[act->costume] * 2 + 2);
        uint8_t numSprites = costlens[offset + frame + 2] + 1;
        //HROOM sprdata = seekResource(&costumes[costdata_id]);
        // DEBUG_PRINTF("Costume %d size %d\n", costdata_id, sss);
        // offset is the beginning
        // in scummvm data is decoded in backwards direction, from the end
        uint16_t sprOffs = READ_LE_UINT16(costoffs + 2 * (offset + frame) + 2) + 2;
        //DEBUG_PRINTF("decode frame=%u numspr=%u offset=%u\n", frame, numSprites, offset);
        //esx_f_seek(sprdata, sprOffs, ESX_SEEK_FWD);
        uint8_t *sprite = sprdata + sprOffs;

        // bool flipped = (newDirToOldDir(a->getFacing()) == 1);
        // int left = 239, right = 0, top = 239, bottom = 0;
        // byte *maskBuf = _vm->getMaskBuffer(0, 0, 1);

        uint8_t anchor = nextSprite;
        animation->anchors[f] = nextSprite;

        // setup anchor and relative sprite attributes and patterns

        for (uint8_t spr = 0 ; spr < numSprites ; spr++)
        {
            uint8_t d0 = *sprite++;
            uint8_t d1 = *sprite++;
            uint8_t d2 = *sprite++;
            uint8_t mask;
            uint8_t sprpal = 0;
            int8_t y, x;

            mask = (d0 & 0x80) ? 0x01 : 0x80;

            y = d0 << 1;
            y >>= 1;

            uint8_t tile = d1;

            sprpal = (d2 & 0x03) << 2;
            x = d2;
            x >>= 2;

        	if (flipped) {
        		mask ^= 0x81;//(mask == 0x80) ? 0x01 : 0x80;
        		x = -x;
        	}

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
            // x is always zero for anchor
            // for relative this attribute is zero too
            IO_SPRITE_ATTRIBUTE = 0;//x < 0 ? 1 : 0;

            if (nextSprite == anchor)
            {
                // invisible yet
                IO_SPRITE_ATTRIBUTE = 0x40 | (nextSprite & 0x3f);
                // anchor 4-bit sprite
                IO_SPRITE_ATTRIBUTE = 0x80;
            }
            else
            {
                IO_SPRITE_ATTRIBUTE = 0xc0 | (nextSprite & 0x3f);
                // relative sprite
                IO_SPRITE_ATTRIBUTE = 0x40;
            }

            //DEBUG_PRINTF("Sprite %d tile=%d/%d x=%d y=%d\n", nextSprite, tile, flipped, x, y);

            ++nextSprite;
        }
    }

	// DEBUG_PRINTF("Decode costume %u size=%u\n", costume, size);
    // for (uint8_t i = 0 ; i < 16 ; ++i)
    //     DEBUG_PRINTF("%x ", readWord(r));
    // DEBUG_PUTS("\n");

    closeRoom(src);

    return nextSprite;
}

void costume_loadData(void)
{
    uint8_t page0 = ZXN_READ_MMU0();
    ZXN_WRITE_MMU0(COST_PAGE0);
    uint8_t page1 = ZXN_READ_MMU1();
    ZXN_WRITE_MMU1(COST_PAGE1);

    HROOM cost = seekResource(&costumes[31]);
    readResource(cost, costume31, sizeof(costume31));
    closeRoom(cost);

    cost = seekResource(&costumes[32]);
    readResource(cost, costume32, sizeof(costume32));
    closeRoom(cost);

    ZXN_WRITE_MMU0(page0);
    ZXN_WRITE_MMU1(page1);
}

void costume_updateAll(void)
{
    uint8_t i;

    // we always need a sprite for the cursor
    graphics_loadSpritePattern(SPRITE_CURSOR,
        // some hack for cursor id
        costdata_id == 32 ? 0xfe : 0xfa, 0x80, 1);

    uint8_t page0 = ZXN_READ_MMU0();
    ZXN_WRITE_MMU0(COST_PAGE0);
    uint8_t page1 = ZXN_READ_MMU1();
    ZXN_WRITE_MMU1(COST_PAGE1);

    // for (i = 1 ; i < 64 ; ++i)
    // {
    //     graphics_loadSpritePattern(i, i + 192, 0x80, 0);
    //     // send attributes
    //     IO_SPRITE_SLOT = i;
    //     IO_SPRITE_ATTRIBUTE = (i & 0xf) << 4; // x
    //     IO_SPRITE_ATTRIBUTE = (i & 0xf0) + 0x20; // y
    //     IO_SPRITE_ATTRIBUTE = 0;
    //     IO_SPRITE_ATTRIBUTE = 0xc0 | (i & 0x3f);
    //     IO_SPRITE_ATTRIBUTE = 0x80 | (i & 0x40); // anchor 4-bit sprite
    // }
    // return;
    // DEBUG_HALT;

    // decode costume sprites
    uint8_t nextSprite = 1;
    for (i = 0 ; i < ACTOR_COUNT ; ++i)
    {
        if (currentRoom && actors[i].room == currentRoom)
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

    ZXN_WRITE_MMU0(page0);
    ZXN_WRITE_MMU1(page1);
}
