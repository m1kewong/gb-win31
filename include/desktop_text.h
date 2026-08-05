#ifndef GBW_DESKTOP_TEXT_H
#define GBW_DESKTOP_TEXT_H

#include <gb/gb.h>

/* Tile 120 is the pointer sprite. Desktop text owns this bank-0 range. */
#define DESKTOP_TEXT_TILE_FIRST 121u
#define DESKTOP_TEXT_TILE_LIMIT 181u
#define DESKTOP_TEXT_TILE_COUNT 47u
#define DESKTOP_TEXT_LABEL_TILES 4u

typedef enum DesktopTextId {
    DESKTOP_TEXT_GB_WORKBENCH = 0,
    DESKTOP_TEXT_FILE_OPTIONS_HELP,
    DESKTOP_TEXT_ACCESSORIES,
    DESKTOP_TEXT_PAINT,
    DESKTOP_TEXT_PIANO,
    DESKTOP_TEXT_MEDIA,
    DESKTOP_TEXT_GAMES,
    DESKTOP_TEXT_SWEEPER,
    DESKTOP_TEXT_CANNON,
    DESKTOP_TEXT_COUNT
} DesktopTextId;

/* Call while the LCD is off. This uploads every fixed desktop string. */
void desktop_text_load(void);

/* Draw a preloaded string at background tile coordinates with a CGB palette. */
void desktop_text_draw(
    UINT8 tile_x,
    UINT8 tile_y,
    DesktopTextId id,
    UINT8 palette
);

UINT8 desktop_text_width_tiles(DesktopTextId id);

#endif
