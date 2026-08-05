#ifndef GBW_UI_H
#define GBW_UI_H

#include <gb/gb.h>
#include "input.h"

#define SCREEN_TILES_W 20u
#define SCREEN_TILES_H 18u

typedef struct PointerState {
    UINT8 x;
    UINT8 y;
    UINT8 visible;
} PointerState;

void ui_init(void);
void ui_scene_begin(void);
void ui_scene_end(void);
void ui_clear(UINT8 palette);
void ui_set_tile(UINT8 x, UINT8 y, UINT8 tile, UINT8 palette);
void ui_set_tile_attr(UINT8 x, UINT8 y, UINT8 tile, UINT8 attributes);
void ui_fill(UINT8 x, UINT8 y, UINT8 w, UINT8 h, UINT8 tile, UINT8 palette);
void ui_text(UINT8 x, UINT8 y, const char *text, UINT8 palette);
void ui_text_clipped(UINT8 x, UINT8 y, const char *text, UINT8 palette, UINT8 width);
void ui_window(UINT8 x, UINT8 y, UINT8 w, UINT8 h, const char *title, UINT8 active);
void ui_menu(UINT8 x, UINT8 y, UINT8 width, const char *items);
void ui_icon(UINT8 x, UINT8 y, UINT8 first_tile, const char *label, UINT8 selected);

void pointer_reset(UINT8 x, UINT8 y);
void pointer_show(void);
void pointer_hide(void);
void pointer_update(const InputState *input);
void pointer_move_to(UINT8 x, UINT8 y);
const PointerState *pointer_get(void);
UINT8 pointer_hits(UINT8 x, UINT8 y, UINT8 w, UINT8 h);

#endif
