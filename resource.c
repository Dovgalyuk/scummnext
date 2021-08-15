#include <arch/zxn.h>
#include <arch/zxn/esxdos.h>
#include "resource.h"
#include "debug.h"

Resource costumes[_numCostumes];
Resource scripts[_numScripts];

#define ENC_BYTE 0xff

uint8_t readByte(HROOM f)
{
    uint8_t b;
    esx_f_read(f, &b, 1);
    return b ^ ENC_BYTE;
}

uint16_t readWord(HROOM f)
{
    uint8_t b[2];
    esx_f_read(f, b, 2);
    return (b[0] ^ ENC_BYTE) + (b[1] ^ ENC_BYTE) * 0x100;
}

HROOM openRoom(uint8_t i)
{
    char fname[] = "MM/00.LFL";
    fname[3] += i / 10;
    fname[4] += i % 10;
    return esx_f_open(fname, ESX_MODE_OPEN_EXIST | ESX_MODE_READ);
}

void closeRoom(HROOM r)
{
    esx_f_close(r);
}

HROOM seekResource(Resource *res)
{
    //DEBUG_PRINTF("seek res %d %d\n", res->room, res->roomoffs);
    HROOM f = openRoom(res->room);
    esx_f_seek(f, res->roomoffs, ESX_SEEK_SET);

    return f;
}

void readBuffer(HROOM r, uint8_t *buf, uint16_t sz)
{
    esx_f_read(r, buf, sz);
    while (sz--)
    {
        *buf++ ^= ENC_BYTE;
    }
}


uint16_t readResource(HROOM r, uint8_t *buf, uint16_t sz)
{
    uint16_t size = readWord(r);
    //DEBUG_PRINTF("read res %d into buf %d\n", size, sz);
    uint16_t i;
    if (size > sz)
    {
        DEBUG_PRINTF("Can't load resource with size %u into buffer with size %u\n", size, sz);
        DEBUG_HALT;
    }
    esx_f_seek(r, 2, ESX_SEEK_BWD);

    readBuffer(r, buf, size);

    return size;
}

void seekToOffset(HROOM r, uint16_t offs)
{
    esx_f_seek(r, offs, ESX_SEEK_SET);
}

void seekFwd(HROOM r, uint16_t offs)
{
    esx_f_seek(r, offs, ESX_SEEK_FWD);
}

uint8_t readString(HROOM r, char *s)
{
    char c;
    char *start = s;
    do
    {
        c = readByte(r);
        *s++ = c & 0x7f;
        if (c & 0x80)
        {
            *s++ = ' ';
        }
    }
    while (c);
    return s - start - 1;
}
