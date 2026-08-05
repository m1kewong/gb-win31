#include <gb/gb.h>
#include <gb/cgb.h>

#include "assets.h"
#include "ui.h"

static PointerState pointer_state;

void ui_scene_begin(void)
{
    DISPLAY_OFF;
}

void ui_scene_end(void)
{
    DISPLAY_ON;
}

void ui_set_tile_attr(UINT8 x, UINT8 y, UINT8 tile, UINT8 attributes)
{
    if (x >= SCREEN_TILES_W || y >= SCREEN_TILES_H) return;
    VBK_REG = VBK_BANK_0;
    set_bkg_tile_xy(x, y, tile);
    set_bkg_attribute_xy(x, y, attributes);
    VBK_REG = VBK_BANK_0;
}

void ui_set_tile(UINT8 x, UINT8 y, UINT8 tile, UINT8 palette)
{
    ui_set_tile_attr(x, y, tile, (UINT8)(palette & 7u));
}

void ui_fill(UINT8 x, UINT8 y, UINT8 w, UINT8 h, UINT8 tile, UINT8 palette)
{
    UINT8 ix;
    UINT8 iy;
    if (x >= SCREEN_TILES_W || y >= SCREEN_TILES_H || w == 0u || h == 0u) return;
    if (w > (UINT8)(SCREEN_TILES_W - x)) w = (UINT8)(SCREEN_TILES_W - x);
    if (h > (UINT8)(SCREEN_TILES_H - y)) h = (UINT8)(SCREEN_TILES_H - y);

    for (iy = 0u; iy < h; ++iy) {
        for (ix = 0u; ix < w; ++ix) {
            ui_set_tile((UINT8)(x + ix), (UINT8)(y + iy), tile, palette);
        }
    }
}

void ui_clear(UINT8 palette)
{
    ui_fill(0u, 0u, SCREEN_TILES_W, SCREEN_TILES_H, TILE_BLANK, palette);
}

void ui_text(UINT8 x, UINT8 y, const char *text, UINT8 palette)
{
    if (y >= SCREEN_TILES_H) return;
    while (*text != '\0' && x < SCREEN_TILES_W) {
        ui_set_tile(x, y, assets_font_tile(*text), palette);
        ++x;
        ++text;
    }
}

void ui_text_clipped(UINT8 x, UINT8 y, const char *text, UINT8 palette, UINT8 width)
{
    UINT8 drawn = 0u;
    if (y >= SCREEN_TILES_H) return;
    while (*text != '\0' && drawn < width && x < SCREEN_TILES_W) {
        ui_set_tile(x, y, assets_font_tile(*text), palette);
        ++x;
        ++text;
        ++drawn;
    }
    while (drawn < width && x < SCREEN_TILES_W) {
        ui_set_tile(x, y, TILE_BLANK, palette);
        ++x;
        ++drawn;
    }
}

void ui_window(UINT8 x, UINT8 y, UINT8 w, UINT8 h, const char *title, UINT8 active)
{
    UINT8 ix;
    UINT8 iy;
    UINT8 title_palette = active ? PAL_TITLE_ACTIVE : PAL_TITLE_INACTIVE;

    if (x >= SCREEN_TILES_W || y >= SCREEN_TILES_H ||
        w < 6u || h < 4u ||
        w > (UINT8)(SCREEN_TILES_W - x) ||
        h > (UINT8)(SCREEN_TILES_H - y)) return;

    ui_set_tile(x, y, TILE_FRAME_TL, PAL_WINDOW);
    ui_set_tile((UINT8)(x + w - 1u), y, TILE_FRAME_TR, PAL_WINDOW);
    ui_set_tile(x, (UINT8)(y + h - 1u), TILE_FRAME_BL, PAL_WINDOW);
    ui_set_tile((UINT8)(x + w - 1u), (UINT8)(y + h - 1u), TILE_FRAME_BR, PAL_WINDOW);

    for (ix = 1u; ix < (UINT8)(w - 1u); ++ix) {
        ui_set_tile((UINT8)(x + ix), y, TILE_BLANK, title_palette);
        ui_set_tile((UINT8)(x + ix), (UINT8)(y + h - 1u), TILE_FRAME_B, PAL_WINDOW);
    }
    for (iy = 1u; iy < (UINT8)(h - 1u); ++iy) {
        ui_set_tile(x, (UINT8)(y + iy), TILE_FRAME_L, PAL_WINDOW);
        ui_set_tile((UINT8)(x + w - 1u), (UINT8)(y + iy), TILE_FRAME_R, PAL_WINDOW);
    }

    ui_fill((UINT8)(x + 1u), (UINT8)(y + 1u), (UINT8)(w - 2u), (UINT8)(h - 2u), TILE_BLANK, PAL_WINDOW);
    ui_set_tile((UINT8)(x + 1u), y, TILE_SYSTEM_BUTTON, title_palette);
    ui_set_tile((UINT8)(x + w - 3u), y, TILE_MIN_BUTTON, title_palette);
    ui_set_tile((UINT8)(x + w - 2u), y, TILE_MAX_BUTTON, title_palette);
    ui_text_clipped((UINT8)(x + 2u), y, title, title_palette, (UINT8)(w - 5u));
}

void ui_menu(UINT8 x, UINT8 y, UINT8 width, const char *items)
{
    ui_fill(x, y, width, 1u, TILE_BLANK, PAL_WINDOW);
    ui_text(x, y, items, PAL_WINDOW);
}

void ui_icon(UINT8 x, UINT8 y, UINT8 first_tile, const char *label, UINT8 selected)
{
    UINT8 length = 0u;
    UINT8 label_x;
    UINT8 offset;
    const char *scan = label;

    if (x + 1u >= SCREEN_TILES_W || y + 2u >= SCREEN_TILES_H) return;

    ui_set_tile(x, y, first_tile, PAL_ICON);
    ui_set_tile((UINT8)(x + 1u), y, (UINT8)(first_tile + 1u), PAL_ICON);
    ui_set_tile(x, (UINT8)(y + 1u), (UINT8)(first_tile + 2u), PAL_ICON);
    ui_set_tile((UINT8)(x + 1u), (UINT8)(y + 1u), (UINT8)(first_tile + 3u), PAL_ICON);

    while (*scan != '\0' && length < SCREEN_TILES_W) { ++length; ++scan; }
    offset = (length < 2u) ? 0u : (UINT8)((length - 2u) >> 1u);
    label_x = (x > offset) ? (UINT8)(x - offset) : 0u;
    if (label_x > (UINT8)(SCREEN_TILES_W - length)) {
        label_x = (UINT8)(SCREEN_TILES_W - length);
    }

    ui_text(label_x, (UINT8)(y + 2u), label,
            selected ? PAL_TITLE_ACTIVE : PAL_WINDOW);
    if (selected && x > 0u) ui_set_tile((UINT8)(x - 1u), y, TILE_CURSOR_MARK, PAL_GAME);
}

void pointer_reset(UINT8 x, UINT8 y)
{
    pointer_state.x = x;
    pointer_state.y = y;
    pointer_state.visible = 0u;
    set_sprite_tile(0u, TILE_POINTER_SPRITE);
    set_sprite_prop(0u, 0u);
    move_sprite(0u, (UINT8)(x + 8u), (UINT8)(y + 16u));
}

void pointer_show(void)
{
    pointer_state.visible = 1u;
    move_sprite(0u, (UINT8)(pointer_state.x + 8u), (UINT8)(pointer_state.y + 16u));
    SHOW_SPRITES;
}

void pointer_hide(void)
{
    pointer_state.visible = 0u;
    move_sprite(0u, 0u, 0u);
}

void pointer_update(const InputState *input)
{
    if (input->held & J_LEFT) {
        if (pointer_state.x > 0u) --pointer_state.x;
    }
    if (input->held & J_RIGHT) {
        if (pointer_state.x < 152u) ++pointer_state.x;
    }
    if (input->held & J_UP) {
        if (pointer_state.y > 0u) --pointer_state.y;
    }
    if (input->held & J_DOWN) {
        if (pointer_state.y < 136u) ++pointer_state.y;
    }
    if (pointer_state.visible) {
        move_sprite(0u, (UINT8)(pointer_state.x + 8u), (UINT8)(pointer_state.y + 16u));
    }
}

void pointer_move_to(UINT8 x, UINT8 y)
{
    pointer_state.x = (x > 152u) ? 152u : x;
    pointer_state.y = (y > 136u) ? 136u : y;
    if (pointer_state.visible) {
        move_sprite(0u, (UINT8)(pointer_state.x + 8u), (UINT8)(pointer_state.y + 16u));
    }
}

const PointerState *pointer_get(void)
{
    return &pointer_state;
}

UINT8 pointer_hits(UINT8 x, UINT8 y, UINT8 w, UINT8 h)
{
    return (UINT8)(pointer_state.x >= x && pointer_state.x < (UINT8)(x + w) &&
                   pointer_state.y >= y && pointer_state.y < (UINT8)(y + h));
}

void ui_init(void)
{
    DISPLAY_OFF;
    SPRITES_8x8;
    assets_load();
    pointer_reset(80u, 72u);
    ui_clear(PAL_DESKTOP);
    SHOW_BKG;
    DISPLAY_ON;
}
