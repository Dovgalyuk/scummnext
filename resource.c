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
    HROOM f = openRoom(res->room);
    esx_f_seek(f, res->roomoffs, ESX_SEEK_SET);

    return f;
}

uint16_t readResource(HROOM r, uint8_t *buf, uint16_t sz)
{
    uint16_t size = readWord(r);
    uint16_t i;
    if (size > sz)
    {
        DEBUG_PRINTF("Can't load resource with size %u into buffer with size %u\n", size, sz);
        DEBUG_HALT;
    }
    esx_f_seek(r, 2, ESX_SEEK_BWD);
    esx_f_read(r, buf, size);
    i = size;
    while (i)
    {
        buf[--i] ^= ENC_BYTE;
    }

    return size;
}
