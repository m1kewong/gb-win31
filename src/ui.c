#pragma bank 0

#include <gb/gb.h>
#include <gb/cgb.h>

#include "assets.h"
#include "ui.h"

static PointerState pointer_state;
static UINT8 label_pool_used;
static UINT8 art_buffer[16];
char ui_art_scratch[64];

void ui_scene_begin(void)
{
    DISPLAY_OFF;
    label_pool_used = 0u;
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

void ui_set_art(UINT8 x, UINT8 y, UINT8 tile, UINT8 palette)
{
    ui_set_tile_attr(x, y, tile, (UINT8)(BKGF_BANK1 | (palette & 7u)));
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

/* Art strings hold 64 characters '0'-'3' per tile, row-major. */
void ui_art_load(UINT8 vram_bank, UINT8 first_tile, UINT8 count, const char *art)
{
    UINT8 tile;
    UINT8 row;
    UINT8 column;
    UINT8 color;
    UINT8 plane0;
    UINT8 plane1;
    UINT8 mask;

    VBK_REG = vram_bank ? VBK_BANK_1 : VBK_BANK_0;
    for (tile = 0u; tile != count; ++tile) {
        for (row = 0u; row != 8u; ++row) {
            plane0 = 0u;
            plane1 = 0u;
            mask = 0x80u;
            for (column = 0u; column != 8u; ++column) {
                color = (UINT8)(*art - '0');
                if (color & 1u) plane0 |= mask;
                if (color & 2u) plane1 |= mask;
                mask >>= 1u;
                ++art;
            }
            art_buffer[(UINT8)(row << 1u)] = plane0;
            art_buffer[(UINT8)((row << 1u) + 1u)] = plane1;
        }
        set_bkg_data((UINT8)(first_tile + tile), 1u, art_buffer);
    }
    VBK_REG = VBK_BANK_0;
}

static UINT8 ui_font_tile(char c)
{
    UINT8 value = (UINT8)c;
    if (value >= (UINT8)'a' && value <= (UINT8)'z') value = (UINT8)(value - 32u);
    if (value < 32u || value > 95u) value = (UINT8)'?';
    return (UINT8)(value - 32u);
}

void ui_text(UINT8 x, UINT8 y, const char *text, UINT8 palette)
{
    if (y >= SCREEN_TILES_H) return;
    while (*text != '\0' && x < SCREEN_TILES_W) {
        ui_set_tile(x, y, ui_font_tile(*text), palette);
        ++x;
        ++text;
    }
}

UINT8 ui_label_init(UiLabel *label, UINT8 x, UINT8 y, UINT8 width, UINT8 palette)
{
    UINT8 i;

    if (width > TEXT_MAX_TILES) width = TEXT_MAX_TILES;
    if (x >= SCREEN_TILES_W || y >= SCREEN_TILES_H ||
        width > (UINT8)(SCREEN_TILES_W - x) ||
        width > (UINT8)(LABEL_POOL_COUNT - label_pool_used)) {
        label->width = 0u;
        return 0u;
    }

    label->x = x;
    label->y = y;
    label->width = width;
    label->tile = (UINT8)(LABEL_POOL_FIRST + label_pool_used);
    label_pool_used = (UINT8)(label_pool_used + width);
    for (i = 0u; i != width; ++i) {
        ui_set_art((UINT8)(x + i), y, (UINT8)(label->tile + i), palette);
    }
    return 1u;
}

void ui_label_set(const UiLabel *label, const char *text, UINT8 colors, UINT8 flags)
{
    if (label->width == 0u) return;
    text_render(label->width, text, 1u, colors, flags);
    text_upload(1u, label->tile, label->width);
}

void ui_label_palette(const UiLabel *label, UINT8 palette)
{
    UINT8 i;
    for (i = 0u; i != label->width; ++i) {
        ui_set_art((UINT8)(label->x + i), label->y, (UINT8)(label->tile + i), palette);
    }
}

void ui_label(UINT8 x, UINT8 y, UINT8 width, const char *text, UINT8 palette,
              UINT8 colors, UINT8 flags)
{
    UiLabel label;
    if (ui_label_init(&label, x, y, width, palette)) {
        ui_label_set(&label, text, colors, flags);
    }
}

void ui_window(UINT8 x, UINT8 y, UINT8 w, UINT8 h, const char *title,
               UINT8 active, UINT8 client)
{
    UINT8 iy;
    UINT8 face = (UINT8)(client == UI_CLIENT_FACE);
    UINT8 fill = face ? TILE_FACE : TILE_BLANK;

    if (x >= SCREEN_TILES_W || y >= SCREEN_TILES_H ||
        w < 6u || h < 3u ||
        w > (UINT8)(SCREEN_TILES_W - x) ||
        h > (UINT8)(SCREEN_TILES_H - y)) return;

    ui_set_tile(x, y, TILE_FRAME_TL, PAL_WINDOW);
    ui_set_tile((UINT8)(x + 1u), y, TILE_SYSTEM_BUTTON, PAL_WINDOW);
    if (title == (const char *)0) {
        ui_fill((UINT8)(x + 2u), y, (UINT8)(w - 5u), 1u, TILE_BLANK,
                active ? PAL_TITLE_ACTIVE : PAL_TITLE_INACTIVE);
    } else {
        ui_label((UINT8)(x + 2u), y, (UINT8)(w - 5u), title,
                 active ? PAL_TITLE_ACTIVE : PAL_TITLE_INACTIVE,
                 TEXT_INK_ON_PAPER, TEXT_ALIGN_CENTER);
    }
    ui_set_tile((UINT8)(x + w - 3u), y, TILE_MIN_BUTTON, PAL_WINDOW);
    ui_set_tile((UINT8)(x + w - 2u), y, TILE_MAX_BUTTON, PAL_WINDOW);
    ui_set_tile((UINT8)(x + w - 1u), y, TILE_FRAME_TR, PAL_WINDOW);

    for (iy = 1u; iy < (UINT8)(h - 1u); ++iy) {
        ui_set_tile(x, (UINT8)(y + iy), face ? TILE_FRAME_L_FACE : TILE_FRAME_L, PAL_WINDOW);
        ui_set_tile((UINT8)(x + w - 1u), (UINT8)(y + iy),
                    face ? TILE_FRAME_R_FACE : TILE_FRAME_R, PAL_WINDOW);
    }
    ui_fill((UINT8)(x + 1u), (UINT8)(y + 1u), (UINT8)(w - 2u), (UINT8)(h - 2u),
            fill, PAL_WINDOW);

    ui_set_tile(x, (UINT8)(y + h - 1u), face ? TILE_FRAME_BL_FACE : TILE_FRAME_BL, PAL_WINDOW);
    ui_fill((UINT8)(x + 1u), (UINT8)(y + h - 1u), (UINT8)(w - 2u), 1u,
            face ? TILE_FRAME_B_FACE : TILE_FRAME_B, PAL_WINDOW);
    ui_set_tile((UINT8)(x + w - 1u), (UINT8)(y + h - 1u),
                face ? TILE_FRAME_BR_FACE : TILE_FRAME_BR, PAL_WINDOW);
}

void ui_menu(UINT8 x, UINT8 y, UINT8 width, const char *items)
{
    ui_label(x, y, width, items, PAL_WINDOW, TEXT_INK_ON_PAPER,
             (UINT8)(TEXT_TOP | TEXT_RULE_BOTTOM));
}

void ui_status(UINT8 x, UINT8 y, UINT8 width, const char *text)
{
    ui_label(x, y, width, text, PAL_WINDOW, TEXT_COLORS(COLOR_GREY, COLOR_BLACK, 0u), 0u);
}

/* Draw a sunken border whose inside is the rectangle (x, y, w, h). */
void ui_sunken(UINT8 x, UINT8 y, UINT8 w, UINT8 h)
{
    UINT8 i;
    UINT8 left = (UINT8)(x - 1u);
    UINT8 top = (UINT8)(y - 1u);
    UINT8 right = (UINT8)(x + w);
    UINT8 bottom = (UINT8)(y + h);

    ui_set_tile(left, top, TILE_SUNK_TL, PAL_WINDOW);
    ui_set_tile(right, top, TILE_SUNK_TR, PAL_WINDOW);
    ui_set_tile(left, bottom, TILE_SUNK_BL, PAL_WINDOW);
    ui_set_tile(right, bottom, TILE_SUNK_BR, PAL_WINDOW);
    for (i = 0u; i != w; ++i) {
        ui_set_tile((UINT8)(x + i), top, TILE_SUNK_T, PAL_WINDOW);
        ui_set_tile((UINT8)(x + i), bottom, TILE_SUNK_B, PAL_WINDOW);
    }
    for (i = 0u; i != h; ++i) {
        ui_set_tile(left, (UINT8)(y + i), TILE_SUNK_L, PAL_WINDOW);
        ui_set_tile(right, (UINT8)(y + i), TILE_SUNK_R, PAL_WINDOW);
    }
}

/* Segment bits: a b c d e f g (bit 6 down to bit 0). */
static const UINT8 led_segments[10] = {
    0x7eu, 0x30u, 0x6du, 0x79u, 0x33u, 0x5bu, 0x5fu, 0x70u, 0x7fu, 0x7bu
};

static void led_set(UINT8 *tile, UINT8 row, UINT8 mask)
{
    /* Background is colour 3 (both planes set); lit pixels are colour 2. */
    tile[(UINT8)(row << 1u)] &= (UINT8)~mask;
}

void ui_led_load(UINT8 first_tile)
{
    UINT8 digit;
    UINT8 half;
    UINT8 row;
    UINT8 segments;
    UINT8 y;

    VBK_REG = VBK_BANK_1;
    for (digit = 0u; digit != 10u; ++digit) {
        segments = led_segments[digit];
        for (half = 0u; half != 2u; ++half) {
            for (row = 0u; row != 16u; ++row) art_buffer[row] = 0xffu;
            for (row = 0u; row != 8u; ++row) {
                y = (UINT8)(row + (half << 3u));
                if (y == 1u && (segments & 0x40u)) led_set(art_buffer, row, 0x3cu);
                if (y >= 2u && y <= 6u && (segments & 0x20u)) led_set(art_buffer, row, 0x02u);
                if (y >= 8u && y <= 12u && (segments & 0x10u)) led_set(art_buffer, row, 0x02u);
                if (y == 13u && (segments & 0x08u)) led_set(art_buffer, row, 0x3cu);
                if (y >= 8u && y <= 12u && (segments & 0x04u)) led_set(art_buffer, row, 0x40u);
                if (y >= 2u && y <= 6u && (segments & 0x02u)) led_set(art_buffer, row, 0x40u);
                if (y == 7u && (segments & 0x01u)) led_set(art_buffer, row, 0x3cu);
            }
            set_bkg_data((UINT8)(first_tile + (UINT8)(digit << 1u) + half), 1u, art_buffer);
        }
    }
    VBK_REG = VBK_BANK_0;
}

void ui_led_draw(UINT8 x, UINT8 y, UINT8 first_tile, UINT16 value,
                 UINT8 digits, UINT8 palette)
{
    UINT8 digit;
    UINT8 column = digits;

    while (column != 0u) {
        --column;
        digit = (UINT8)(value % 10u);
        value /= 10u;
        ui_set_art((UINT8)(x + column), y, (UINT8)(first_tile + (UINT8)(digit << 1u)), palette);
        ui_set_art((UINT8)(x + column), (UINT8)(y + 1u),
                   (UINT8)(first_tile + (UINT8)(digit << 1u) + 1u), palette);
    }
}

#define POINTER_SPRITE_TOP 0u
#define POINTER_SPRITE_TAIL 1u

static void pointer_place(void)
{
    UINT8 screen_x = (UINT8)(pointer_state.x + 8u);
    UINT8 screen_y = (UINT8)(pointer_state.y + 16u);

    move_sprite(POINTER_SPRITE_TOP, screen_x, screen_y);
    move_sprite(POINTER_SPRITE_TAIL, screen_x, (UINT8)(screen_y + 8u));
}

void pointer_reset(UINT8 x, UINT8 y)
{
    pointer_state.x = x;
    pointer_state.y = y;
    pointer_state.visible = 0u;
    set_sprite_tile(POINTER_SPRITE_TOP, TILE_POINTER_SPRITE);
    set_sprite_tile(POINTER_SPRITE_TAIL, (UINT8)(TILE_POINTER_SPRITE + 1u));
    set_sprite_prop(POINTER_SPRITE_TOP, 0u);
    set_sprite_prop(POINTER_SPRITE_TAIL, 0u);
    pointer_place();
}

void pointer_show(void)
{
    pointer_state.visible = 1u;
    pointer_place();
    SHOW_SPRITES;
}

void pointer_hide(void)
{
    pointer_state.visible = 0u;
    move_sprite(POINTER_SPRITE_TOP, 0u, 0u);
    move_sprite(POINTER_SPRITE_TAIL, 0u, 0u);
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
    if (pointer_state.visible) pointer_place();
}

void pointer_move_to(UINT8 x, UINT8 y)
{
    pointer_state.x = (x > 152u) ? 152u : x;
    pointer_state.y = (y > 136u) ? 136u : y;
    if (pointer_state.visible) pointer_place();
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

UINT8 pointer_in_tiles(UINT8 x, UINT8 y, UINT8 w, UINT8 h)
{
    return pointer_hits((UINT8)(x << 3u), (UINT8)(y << 3u),
                        (UINT8)(w << 3u), (UINT8)(h << 3u));
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
