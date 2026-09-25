#ifndef GBW_ASSETS_H
#define GBW_ASSETS_H

#include <gb/gb.h>
#include <gb/cgb.h>

/* Palettes 0, 1, 2 and 4 are system palettes and never change. Scenes may
 * reload the app palettes (3, 5, 6, 7) on entry; the desktop restores them. */
enum BackgroundPalette {
    PAL_DESKTOP = 0,
    PAL_WINDOW = 1,
    PAL_TITLE_ACTIVE = 2,
    PAL_APP_A = 3,
    PAL_ICON = 4,
    PAL_APP_B = 5,
    PAL_MONO = 6,
    PAL_APP_C = 7
};

/* Windows 3.1 draws inactive titles as black text on white. */
#define PAL_TITLE_INACTIVE PAL_WINDOW

/* PAL_WINDOW colour indices. */
#define COLOR_WHITE 0u
#define COLOR_GREY 1u
#define COLOR_DARK 2u
#define COLOR_BLACK 3u

/* Bank-0 tiles. The two pointer sprite tiles (120-121) share the background
 * tile space. */
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
    TILE_FACE = 78,
    TILE_CANNON = 79,
    TILE_TARGET = 80,
    TILE_SUNK_TL = 81,
    TILE_SUNK_T = 82,
    TILE_SUNK_TR = 83,
    TILE_SUNK_L = 84,
    TILE_SUNK_R = 85,
    TILE_SUNK_BL = 86,
    TILE_SUNK_B = 87,
    TILE_SUNK_BR = 88,
    TILE_FRAME_L_FACE = 89,
    TILE_FRAME_R_FACE = 90,
    TILE_FRAME_BL_FACE = 91,
    TILE_FRAME_B_FACE = 92,
    TILE_FRAME_BR_FACE = 93,
    TILE_ICON_FIRST = 96,
    TILE_ICON_PAINT = 96,
    TILE_ICON_PIANO = 100,
    TILE_ICON_MEDIA = 104,
    TILE_ICON_SWEEPER = 108,
    TILE_ICON_CANNON = 112,
    TILE_ICON_COUNT = 20,
    TILE_STATIC_COUNT = 116
};

#define TILE_POINTER_SPRITE 120u
#define TILE_POINTER_SPRITE_COUNT 2u

/* VRAM bank 1: per-scene application art, then the per-scene label pool. */
#define APP_ART_FIRST 0u
#define APP_ART_COUNT 144u
#define LABEL_POOL_FIRST 144u
#define LABEL_POOL_COUNT 112u

void assets_load(void) BANKED;
void assets_restore_palettes(void) BANKED;

#endif
