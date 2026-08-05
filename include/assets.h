#ifndef GBW_ASSETS_H
#define GBW_ASSETS_H

#include <gb/gb.h>
#include <gb/cgb.h>

enum BackgroundPalette {
    PAL_DESKTOP = 0,
    PAL_WINDOW = 1,
    PAL_TITLE_ACTIVE = 2,
    PAL_TITLE_INACTIVE = 3,
    PAL_ICON = 4,
    PAL_GAME = 5,
    PAL_MONO = 6,
    PAL_GAME_FOCUS = 7
};

enum BackgroundTile {
    TILE_FONT_FIRST = 0,
    TILE_FONT_LAST = 63,
    TILE_BLANK = 64,
    TILE_SOLID = 65,
    TILE_FRAME_TL = 66,
    TILE_FRAME_T = 67,
    TILE_FRAME_TR = 68,
    TILE_FRAME_L = 69,
    TILE_FRAME_R = 70,
    TILE_FRAME_BL = 71,
    TILE_FRAME_B = 72,
    TILE_FRAME_BR = 73,
    TILE_SYSTEM_BUTTON = 74,
    TILE_MIN_BUTTON = 75,
    TILE_MAX_BUTTON = 76,
    TILE_CURSOR_MARK = 77,
    TILE_MS_HIDDEN = 78,
    TILE_MS_FLAG = 79,
    TILE_MS_MINE = 80,
    TILE_MS_EXPLODED = 81,
    TILE_MS_NUM_1 = 82,
    TILE_MS_NUM_8 = 89,
    TILE_KEY_WHITE = 90,
    TILE_KEY_WHITE_ACTIVE = 91,
    TILE_KEY_BLACK = 92,
    TILE_CANNON = 93,
    TILE_TARGET = 94,
    TILE_ICON_FIRST = 96,
    TILE_ICON_PAINT = 96,
    TILE_ICON_PIANO = 100,
    TILE_ICON_MEDIA = 104,
    TILE_ICON_SWEEPER = 108,
    TILE_ICON_CANNON = 112,
    TILE_ICON_COUNT = 20,
    TILE_STATIC_COUNT = 116
};

#define TILE_POINTER_SPRITE 118u
#define TILE_POINTER_SPRITE_COUNT 2u

void assets_load(void);
UINT8 assets_font_tile(char c);

#endif
