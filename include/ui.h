#ifndef GBW_UI_H
#define GBW_UI_H

#include <gb/gb.h>
#include "input.h"
#include "text.h"

#define SCREEN_TILES_W 20u
#define SCREEN_TILES_H 18u

#define UI_CLIENT_WHITE 0u
#define UI_CLIENT_FACE 1u

typedef struct PointerState {
    UINT8 x;
    UINT8 y;
    UINT8 visible;
} PointerState;

/* A run of label-pool tiles mapped at a fixed screen position. */
typedef struct UiLabel {
    UINT8 x;
    UINT8 y;
    UINT8 width;
    UINT8 tile;
} UiLabel;

void ui_init(void);
void ui_scene_begin(void);
void ui_scene_end(void);
void ui_clear(UINT8 palette);
void ui_set_tile(UINT8 x, UINT8 y, UINT8 tile, UINT8 palette);
void ui_set_tile_attr(UINT8 x, UINT8 y, UINT8 tile, UINT8 attributes);
void ui_set_art(UINT8 x, UINT8 y, UINT8 tile, UINT8 palette);
void ui_fill(UINT8 x, UINT8 y, UINT8 w, UINT8 h, UINT8 tile, UINT8 palette);
void ui_art_load(UINT8 vram_bank, UINT8 first_tile, UINT8 count, const char *art);

/* One 8x8 art tile of scratch shared by scenes; only one scene runs at a time. */
extern char ui_art_scratch[64];

/* Fixed-pitch 8x8 text for the BIOS and DOS screens. */
void ui_text(UINT8 x, UINT8 y, const char *text, UINT8 palette);

UINT8 ui_label_init(UiLabel *label, UINT8 x, UINT8 y, UINT8 width, UINT8 palette);
void ui_label_set(const UiLabel *label, const char *text, UINT8 colors, UINT8 flags);
void ui_label_palette(const UiLabel *label, UINT8 palette);
void ui_label(UINT8 x, UINT8 y, UINT8 width, const char *text, UINT8 palette,
              UINT8 colors, UINT8 flags);

void ui_window(UINT8 x, UINT8 y, UINT8 w, UINT8 h, const char *title,
               UINT8 active, UINT8 client);
void ui_menu(UINT8 x, UINT8 y, UINT8 width, const char *items);
void ui_status(UINT8 x, UINT8 y, UINT8 width, const char *text);
void ui_sunken(UINT8 x, UINT8 y, UINT8 w, UINT8 h);

/* Seven-segment LED digits, 8x16 each, loaded as 20 bank-1 art tiles:
 * digit d uses tiles first + 2d (top) and first + 2d + 1 (bottom).
 * Colour 3 is the background and colour 2 the lit segments. */
#define UI_LED_TILE_COUNT 20u
void ui_led_load(UINT8 first_tile);
void ui_led_draw(UINT8 x, UINT8 y, UINT8 first_tile, UINT16 value,
                 UINT8 digits, UINT8 palette);

void pointer_reset(UINT8 x, UINT8 y);
void pointer_show(void);
void pointer_hide(void);
void pointer_update(const InputState *input);
void pointer_move_to(UINT8 x, UINT8 y);
const PointerState *pointer_get(void);
UINT8 pointer_hits(UINT8 x, UINT8 y, UINT8 w, UINT8 h);
UINT8 pointer_in_tiles(UINT8 x, UINT8 y, UINT8 w, UINT8 h);

#endif
