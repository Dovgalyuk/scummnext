#include <arch/zxn.h>
#include "cursor.h"
#include "graphics.h"
#include "script.h"

static uint8_t cursorState;
uint16_t cursorX;
uint16_t cursorY;

__sfr __banked __at 0xfbfe IO_FBFE;
__sfr __banked __at 0xfdfe IO_FDFE;
__sfr __banked __at 0xdffe IO_DFFE;

void cursor_setState(uint8_t s)
{
    cursorState = s;
}

void cursor_animate(void)
{
    // color = default_cursor_colors[idx];
    // _cursor.width = 8;
    // _cursor.height = 8;
    // _cursor.hotspotX = 0;
    // _cursor.hotspotY = 0;

    // byte *dst = _grabbedCursor;
    // byte *src = &_NESPatTable[0][0xfa * 16];
    // byte *palette = _NESPalette[1];

    // for (i = 0; i < 8; i++)
    // {
    //     byte c0 = src[i];
    //     byte c1 = src[i + 8];
    //     for (j = 0; j < 8; j++)
    //         *dst++ = palette[((c0 >> (7 - j)) & 1) | (((c1 >> (7 - j)) & 1) << 1) | ((idx == 3) ? 4 : 0)];
    // }

	// CursorMan.replaceCursor(_grabbedCursor, _cursor.width, _cursor.height,
	// 						_cursor.hotspotX, _cursor.hotspotY,
	// 						(_game.platform == Common::kPlatformNES ? _grabbedCursor[63] : transColor),
	// 						(_game.heversion == 70 ? true : false));

    // scan keyboard
    if (!(IO_DFFE & 1))
        ++cursorX;
    if (!(IO_DFFE & 2))
        --cursorX;
    if (!(IO_FBFE & 1))
        --cursorY;
    if (!(IO_FDFE & 1))
        ++cursorY;

    // update scumm vars
    // scummVars[VAR_VIRT_MOUSE_X] = cursorX >> V12_X_SHIFT;
    // scummVars[VAR_VIRT_MOUSE_Y] = cursorY >> V12_Y_SHIFT;

    uint16_t x = cursorX + (LINE_GAP * 8);
    uint16_t y = cursorY + (SCREEN_TOP * 8);
    IO_SPRITE_SLOT = SPRITE_CURSOR;
    IO_SPRITE_ATTRIBUTE = x; // x
    IO_SPRITE_ATTRIBUTE = y; // y
    IO_SPRITE_ATTRIBUTE = x >> 8; // palette and x8
     // sprite and the pattern are equal
    IO_SPRITE_ATTRIBUTE = 0xc0 | SPRITE_CURSOR; // visible, 5th byte, pattern
    IO_SPRITE_ATTRIBUTE = 0x80; // 4-bit sprite and y8
}
