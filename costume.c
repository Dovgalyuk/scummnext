#include <arch/zxn.h>
#include <arch/zxn/esxdos.h>
#include "costume.h"
#include "resource.h"
#include "graphics.h"
#include "room.h"
#include "sprites.h"
#include "helper.h"
#include "debug.h"

#define COST_PAGE0 48
#define COST_PAGE1 49

#define COST_SPRITES 16
#define FIRST_SPRITE 1

extern uint8_t costume31[11234];
extern uint8_t costume32[140];
extern uint8_t actCostumes[2048];
extern uint8_t *costumesPtr[_numCostumes];
extern uint16_t actCostumesUsed;
extern uint8_t anchors[6];

static const uint8_t v1MMNESLookup[25] = {
	0x00, 0x03, 0x01, 0x06, 0x08,
	0x02, 0x00, 0x07, 0x0C, 0x04,
	0x09, 0x0A, 0x12, 0x0B, 0x14,
	0x0D, 0x11, 0x0F, 0x0E, 0x10,
	0x17, 0x00, 0x01, 0x05, 0x16
};

static const uint8_t *getCostume(uint8_t cost)
{
    if (!costumesPtr[cost])
    {
        uint8_t *ptr = actCostumes + actCostumesUsed;
        HROOM src = seekResource(costumes + cost);
        uint16_t sz = readResource(src, ptr, sizeof(actCostumes) - actCostumesUsed);
        closeRoom(src);
        actCostumesUsed += sz;
        DEBUG_ASSERT(actCostumesUsed <= sizeof(actCostumes), "getCostume");
        costumesPtr[cost] = ptr;
    }
    return costumesPtr[cost];
}

static void decodeNESCostume(Actor *act, uint8_t nextSprite)
{
    //DEBUG_PRINTF("Decode for %d\n", nextSprite);

    // costume ID -> v1MMNESLookup[] -> desc -> lens & offs -> data -> Gfx & pal
    // _baseptr = _vm->getResourceAddress(rtCostume, id);
    // _dataOffsets = _baseptr + 2;
    //DEBUG_PRINTF("Decoding costume %d\n", costumes[act->costume]);
    const uint8_t *cost = getCostume(act->costume);

	// int anim;

    //uint16_t size = readWord(src);
    cost += 2;
    //DEBUG_PRINTF("Costume %d for actor %s\n", act->costume, act->name);
    // read data offsets from src

    // limb = 0 for v2 scumm
	// anim = 4 * cost.frame[limb] + newDirToOldDir(a->getFacing());
	// frameNum = cost.curpos[limb];
	// frame = src[src[2 * anim] + frameNum];

    // _numAnim = 0x17 => numframes = 6
    uint8_t anim = act->frame * 4 + act->facing;
    //esx_f_seek(src, 2 * anim, ESX_SEEK_FWD);
    uint8_t begin = cost[2 * anim];//readByte(src);
    uint8_t end = cost[2 * anim + 1];//readByte(src);
    //DEBUG_PRINTF("Decode animation %u/%u of %u frames from %u\n", anim, act->costume, end, begin);
    //DEBUG_ASSERT(end <= MAX_FRAMES, "decodeNESCostume");
    //esx_f_seek(src, begin - 2 * anim - 2, ESX_SEEK_FWD);

    act->frames = end;
    uint8_t *sprdata = costdata_id == 31 ? costume31 : costume32;
    uint8_t flipped = act->facing == 1;
    uint8_t frame = cost[begin + act->curpos];//*cost++;//readByte(src);

    uint16_t offset = READ_LE_UINT16(costdesc + v1MMNESLookup[act->costume] * 2 + 2);
    uint8_t numSprites = costlens[offset + frame + 2] + 1;
    //HROOM sprdata = seekResource(&costumes[costdata_id]);
    //DEBUG_PRINTF("Frame %u numspr %u\n", f, numSprites);
    // offset is the beginning
    // in scummvm data is decoded in backwards direction, from the end
    uint16_t sprOffs = READ_LE_UINT16(costoffs + 2 * (offset + frame) + 2) + 2;
    //DEBUG_PRINTF("decode frame=%u numspr=%u offset=%u\n", frame, numSprites, offset);
    //esx_f_seek(sprdata, sprOffs, ESX_SEEK_FWD);
    uint8_t *sprite = sprdata + sprOffs;

    // bool flipped = (newDirToOldDir(a->getFacing()) == 1);
    // int left = 239, right = 0, top = 239, bottom = 0;
    // byte *maskBuf = _vm->getMaskBuffer(0, 0, 1);

    act->anchor = nextSprite;

    // setup anchor and relative sprite attributes and patterns
    DEBUG_ASSERT(numSprites <= COST_SPRITES, "COST_SPRITES");

    uint8_t spr;
    for (spr = 0 ; spr < numSprites ; spr++)
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
        if (!spr)
        {
            act->ax = x;
            act->ay = y;
        }

        x = x - act->ax;
        y = y - act->ay;
        IO_SPRITE_ATTRIBUTE = x;
        IO_SPRITE_ATTRIBUTE = y;
        // x is always zero for anchor
        // for relative this attribute is zero too
        IO_SPRITE_ATTRIBUTE = 0;//x < 0 ? 1 : 0;

        if (!spr)
        {
            // invisible yet
            IO_SPRITE_ATTRIBUTE = 0x40 | (nextSprite & 0x3f);
            // anchor 4-bit sprite
            IO_SPRITE_ATTRIBUTE = 0x80 | (nextSprite & 0x40);
        }
        else
        {
            // visible and enable 5 byte attribyte
            IO_SPRITE_ATTRIBUTE = 0xc0 | (nextSprite & 0x3f);
            // relative sprite
            IO_SPRITE_ATTRIBUTE = 0x40 | ((nextSprite & 0x40) >> 1);
        }

        //DEBUG_PRINTF("Sprite %d tile=%d/%d x=%d y=%d\n", nextSprite, tile, flipped, x, y);

        ++nextSprite;
    }

    // clean other sprites in the slot
    while (spr < COST_SPRITES)
    {
        IO_SPRITE_SLOT = nextSprite;
        IO_SPRITE_ATTRIBUTE = 0;
        IO_SPRITE_ATTRIBUTE = 0;
        IO_SPRITE_ATTRIBUTE = 0;
        IO_SPRITE_ATTRIBUTE = 0;
        ++nextSprite;
        ++spr;
    }
}

void costume_loadData(void)
{
    PUSH_PAGE(0, COST_PAGE0);
    PUSH_PAGE(1, COST_PAGE1);

    HROOM cost = seekResource(&costumes[31]);
    readResource(cost, costume31, sizeof(costume31));
    closeRoom(cost);

    cost = seekResource(&costumes[32]);
    readResource(cost, costume32, sizeof(costume32));
    closeRoom(cost);

    POP_PAGE(0);
    POP_PAGE(1);
}

void costume_updateAll(void)
{
    uint8_t i;

    // clean all sprites
    for (i = 0 ; i <= 127 ; ++i)
    {
        IO_SPRITE_SLOT = i;
        IO_SPRITE_ATTRIBUTE = 0;
        IO_SPRITE_ATTRIBUTE = 0;
        IO_SPRITE_ATTRIBUTE = 0;
        IO_SPRITE_ATTRIBUTE = 0;
    }

    // we always need a sprite for the cursor
    graphics_loadSpritePattern(SPRITE_CURSOR,
        // some hack for cursor id
        costdata_id == 32 ? 0xfe : 0xfa, 0x80, 1);

    PUSH_PAGE(0, COST_PAGE0);
    PUSH_PAGE(1, COST_PAGE1);

    for (i = 0 ; i < sizeof(anchors) ; ++i)
        anchors[i] = 0;

    //DEBUG_PUTS("Update all costumes\n");

    // decode costume sprites
    uint8_t nextSprite = FIRST_SPRITE;
    uint8_t a = 0;
    for (i = 0 ; i < ACTOR_COUNT ; ++i)
    {
        if (currentRoom && actors[i].room == currentRoom)
        {
            anchors[a] = 1;
            ++a;
            actors[i].old_anchor = 0;
            DEBUG_ASSERT(nextSprite <= 127 && a <= sizeof(anchors), "costume_updateAll");
            decodeNESCostume(&actors[i], nextSprite);
            nextSprite += COST_SPRITES;
        }
    }

    POP_PAGE(0);
    POP_PAGE(1);
}

void costume_updateActor(Actor *act)
{
    PUSH_PAGE(0, COST_PAGE0);
    PUSH_PAGE(1, COST_PAGE1);

    uint8_t anchor = act->anchor;
    uint8_t a = 0;
    if (act->room == currentRoom)
    {
        if (act->old_anchor)
        {
            a = act->old_anchor / COST_SPRITES;
        }
        else
        {
            while (anchors[a])
                ++a;
        }
        DEBUG_ASSERT(a < sizeof(anchors), "costume_updateActor");
        // sets act->anchor
        decodeNESCostume(act, FIRST_SPRITE + a * COST_SPRITES);
        anchors[a] = 1;
    }
    act->old_anchor = anchor;

    //DEBUG_PRINTF("Update costume old=%d new=%d\n", anchor, FIRST_SPRITE + a * COST_SPRITES);

    POP_PAGE(0);
    POP_PAGE(1);
}

void actors_draw(uint8_t offs, uint8_t gap)
{
    PUSH_PAGE(0, COST_PAGE0);
    PUSH_PAGE(1, COST_PAGE1);

    uint8_t i;
    // draw actors
    for (i = 0 ; i < ACTOR_COUNT ; ++i)
    {
        Actor *act = &actors[i];
        if (act->room == currentRoom)
        {
            int16_t xx = act->x * V12_X_MULTIPLIER + act->ax - offs * 8;
            if (act->old_anchor)
            {
                //DEBUG_PRINTF("Hide %d %d\n", i, act->old_anchor);
                // switch old sprite off
                IO_SPRITE_SLOT = act->old_anchor;
                IO_SPRITE_ATTRIBUTE = 0;
                IO_SPRITE_ATTRIBUTE = 0;
                IO_SPRITE_ATTRIBUTE = 0;
                // invisible
                IO_SPRITE_ATTRIBUTE = 0x40;
                IO_SPRITE_ATTRIBUTE = 0x80;
                anchors[act->old_anchor / COST_SPRITES] = 0;
                act->old_anchor = 0;
            }
            uint8_t anchor = act->anchor;
            //DEBUG_PRINTF("Show %d %d\n", i, anchor);
            IO_SPRITE_SLOT = anchor;
            if (xx < 0 || xx > SCREEN_WIDTH * 8)
            {
                IO_SPRITE_ATTRIBUTE = 0;
                IO_SPRITE_ATTRIBUTE = 0;
                IO_SPRITE_ATTRIBUTE = 0;
                // invisible
                IO_SPRITE_ATTRIBUTE = 0x40 | (anchor & 0x3f);
                // anchor 4-bit sprite
                IO_SPRITE_ATTRIBUTE = 0x80 | (anchor & 0x40);
                continue;
            }
            xx += gap * 8;
            uint8_t y = act->y * V12_Y_MULTIPLIER + act->ay + SCREEN_TOP * 8;
            IO_SPRITE_ATTRIBUTE = xx;
            IO_SPRITE_ATTRIBUTE = y;
            IO_SPRITE_ATTRIBUTE = (xx >> 8) & 1;
            IO_SPRITE_ATTRIBUTE = 0xc0 | (anchor & 0x3f);
            IO_SPRITE_ATTRIBUTE = 0x80 | (anchor & 0x40);
            //DEBUG_PRINTF("Actor %u x=%u y=%u\n", i, xx, y);
            //graphics_drawActor(actors + i);
            //a->animateCostume();
        }
    }

    POP_PAGE(0);
    POP_PAGE(1);
}
