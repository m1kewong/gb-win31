#include <gb/gb.h>
#include <gb/cgb.h>
#include <string.h>

#include "assets.h"
#include "audio.h"
#include "paint.h"
#include "ui.h"

#define PAINT_CANVAS_X_TILES 3u
#define PAINT_CANVAS_Y_TILES 4u
#define PAINT_CANVAS_X_PIXELS (PAINT_CANVAS_X_TILES * 8u)
#define PAINT_CANVAS_Y_PIXELS (PAINT_CANVAS_Y_TILES * 8u)
#define PAINT_CANVAS_WIDTH_PIXELS (PAINT_CANVAS_WIDTH_TILES * 8u)
#define PAINT_CANVAS_HEIGHT_PIXELS (PAINT_CANVAS_HEIGHT_TILES * 8u)

#define PAINT_TILE_BYTES 16u
#define PAINT_CANVAS_TILE_COUNT \
    (PAINT_CANVAS_WIDTH_TILES * PAINT_CANVAS_HEIGHT_TILES)
#define PAINT_CANVAS_BYTES (PAINT_CANVAS_TILE_COUNT * PAINT_TILE_BYTES)

/* Bank 1 has its own tile-number namespace, so zero does not alias the font. */
#define PAINT_CANVAS_VRAM_BASE 0u
#define PAINT_SWATCH_VRAM_BASE PAINT_CANVAS_TILE_COUNT
#define PAINT_SHADE_COUNT 4u
#define PAINT_CLEAR_TILES_PER_FRAME 4u

#define PAINT_PRIMARY_SWATCH_X 3u
#define PAINT_SECONDARY_SWATCH_X 7u
#define PAINT_SWATCH_Y 2u

typedef char paint_tiles_must_fit_vram_bank[
    ((PAINT_SWATCH_VRAM_BASE + PAINT_SHADE_COUNT) <= 256u) ? 1 : -1
];

/* 140 independent 8x8, 2bpp tiles. This consumes 2,240 bytes of WRAM. */
static UINT8 paint_canvas[PAINT_CANVAS_BYTES];

static UINT8 paint_initialized;
static UINT8 paint_primary_shade;
static UINT8 paint_secondary_shade;
static UINT8 paint_clear_pending;
static UINT8 paint_clear_next_tile;

/* Four solid 2bpp tiles, one for each shade in PAL_MONO. */
static const UINT8 paint_swatch_tiles[PAINT_SHADE_COUNT * PAINT_TILE_BYTES] = {
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0xffu, 0x00u, 0xffu, 0x00u, 0xffu, 0x00u, 0xffu, 0x00u,
    0xffu, 0x00u, 0xffu, 0x00u, 0xffu, 0x00u, 0xffu, 0x00u,
    0x00u, 0xffu, 0x00u, 0xffu, 0x00u, 0xffu, 0x00u, 0xffu,
    0x00u, 0xffu, 0x00u, 0xffu, 0x00u, 0xffu, 0x00u, 0xffu,
    0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu,
    0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu
};

static UINT16 paint_tile_offset(UINT8 tile)
{
    return (UINT16)tile * PAINT_TILE_BYTES;
}

static void paint_upload_canvas(void)
{
    VBK_REG = VBK_BANK_1;
    set_bkg_data(PAINT_CANVAS_VRAM_BASE, PAINT_CANVAS_TILE_COUNT,
                 paint_canvas);
    set_bkg_data(PAINT_SWATCH_VRAM_BASE, PAINT_SHADE_COUNT,
                 paint_swatch_tiles);
    VBK_REG = VBK_BANK_0;
}

static void paint_upload_tile(UINT8 tile)
{
    VBK_REG = VBK_BANK_1;
    set_bkg_data((UINT8)(PAINT_CANVAS_VRAM_BASE + tile), 1u,
                 &paint_canvas[paint_tile_offset(tile)]);
    VBK_REG = VBK_BANK_0;
}

static void paint_upload_tile_range(UINT8 first, UINT8 count)
{
    VBK_REG = VBK_BANK_1;
    set_bkg_data((UINT8)(PAINT_CANVAS_VRAM_BASE + first), count,
                 &paint_canvas[paint_tile_offset(first)]);
    VBK_REG = VBK_BANK_0;
}

static void paint_map_bank_one_tile(UINT8 x, UINT8 y, UINT8 tile)
{
    set_bkg_tile_xy(x, y, tile);
    set_bkg_attribute_xy(x, y, (UINT8)(BKGF_BANK1 | PAL_MONO));
}

static void paint_map_canvas(void)
{
    UINT8 x;
    UINT8 y;
    UINT8 tile = PAINT_CANVAS_VRAM_BASE;

    for (y = 0u; y != PAINT_CANVAS_HEIGHT_TILES; ++y) {
        for (x = 0u; x != PAINT_CANVAS_WIDTH_TILES; ++x) {
            paint_map_bank_one_tile((UINT8)(PAINT_CANVAS_X_TILES + x),
                                    (UINT8)(PAINT_CANVAS_Y_TILES + y),
                                    tile);
            ++tile;
        }
    }
}

static void paint_draw_canvas_frame(void)
{
    UINT8 x;
    UINT8 y;

    ui_set_tile(2u, 3u, TILE_FRAME_TL, PAL_WINDOW);
    ui_set_tile(17u, 3u, TILE_FRAME_TR, PAL_WINDOW);
    ui_set_tile(2u, 14u, TILE_FRAME_BL, PAL_WINDOW);
    ui_set_tile(17u, 14u, TILE_FRAME_BR, PAL_WINDOW);

    for (x = 3u; x != 17u; ++x) {
        ui_set_tile(x, 3u, TILE_FRAME_T, PAL_WINDOW);
        ui_set_tile(x, 14u, TILE_FRAME_B, PAL_WINDOW);
    }
    for (y = 4u; y != 14u; ++y) {
        ui_set_tile(2u, y, TILE_FRAME_L, PAL_WINDOW);
        ui_set_tile(17u, y, TILE_FRAME_R, PAL_WINDOW);
    }
}

static void paint_draw_swatches(void)
{
    paint_map_bank_one_tile(PAINT_PRIMARY_SWATCH_X, PAINT_SWATCH_Y,
                            (UINT8)(PAINT_SWATCH_VRAM_BASE + paint_primary_shade));
    paint_map_bank_one_tile(PAINT_SECONDARY_SWATCH_X, PAINT_SWATCH_Y,
                            (UINT8)(PAINT_SWATCH_VRAM_BASE + paint_secondary_shade));
}

static void paint_draw_controls(void)
{
    ui_text_clipped(1u, 15u, "SEL:A  A+B:CLEAR", PAL_WINDOW, 18u);
    ui_text_clipped(1u, 16u, "START:DESKTOP", PAL_WINDOW, 18u);
}

static void paint_draw_chrome(void)
{
    ui_clear(PAL_DESKTOP);
    ui_window(0u, 0u, 20u, 18u, "GB PAINT", 1u);
    ui_menu(1u, 1u, 18u, "FILE  HELP");
    ui_text(1u, 2u, "A:", PAL_WINDOW);
    ui_text(5u, 2u, "B:", PAL_WINDOW);
    ui_text(9u, 2u, "SELECT=A", PAL_WINDOW);
    paint_draw_canvas_frame();
    paint_draw_controls();
}

static UINT8 paint_set_pixel(UINT8 x, UINT8 y, UINT8 shade, UINT8 *tile_out)
{
    UINT8 tile_x = (UINT8)(x >> 3u);
    UINT8 tile_y = (UINT8)(y >> 3u);
    UINT8 pixel_x = (UINT8)(x & 7u);
    UINT8 pixel_y = (UINT8)(y & 7u);
    UINT8 tile = (UINT8)(tile_y * PAINT_CANVAS_WIDTH_TILES + tile_x);
    UINT16 offset = (UINT16)(paint_tile_offset(tile) + (UINT16)pixel_y * 2u);
    UINT8 mask = (UINT8)(0x80u >> pixel_x);
    UINT8 old_plane_zero = paint_canvas[offset];
    UINT8 old_plane_one = paint_canvas[(UINT16)(offset + 1u)];
    UINT8 plane_zero = (UINT8)(old_plane_zero & (UINT8)~mask);
    UINT8 plane_one = (UINT8)(old_plane_one & (UINT8)~mask);

    if (shade & 1u) plane_zero |= mask;
    if (shade & 2u) plane_one |= mask;

    *tile_out = tile;
    if (plane_zero == old_plane_zero && plane_one == old_plane_one) return 0u;

    paint_canvas[offset] = plane_zero;
    paint_canvas[(UINT16)(offset + 1u)] = plane_one;
    return 1u;
}

static void paint_draw_at_pointer(UINT8 shade)
{
    const PointerState *pointer = pointer_get();
    UINT8 relative_x;
    UINT8 relative_y;
    UINT8 tile;

    if (pointer->x < PAINT_CANVAS_X_PIXELS ||
        pointer->x >= (UINT8)(PAINT_CANVAS_X_PIXELS + PAINT_CANVAS_WIDTH_PIXELS) ||
        pointer->y < PAINT_CANVAS_Y_PIXELS ||
        pointer->y >= (UINT8)(PAINT_CANVAS_Y_PIXELS + PAINT_CANVAS_HEIGHT_PIXELS)) {
        return;
    }

    relative_x = (UINT8)(pointer->x - PAINT_CANVAS_X_PIXELS);
    relative_y = (UINT8)(pointer->y - PAINT_CANVAS_Y_PIXELS);
    if (paint_set_pixel(relative_x, relative_y, shade, &tile)) {
        paint_upload_tile(tile);
        audio_sfx(SFX_DRAW);
    }
}

static void paint_begin_clear(void)
{
    paint_clear_pending = 1u;
    paint_clear_next_tile = 0u;
    ui_text_clipped(1u, 15u, "CLEARING CANVAS", PAL_WINDOW, 18u);
    audio_sfx(SFX_CLICK);
}

static void paint_clear_step(void)
{
    UINT8 count = PAINT_CLEAR_TILES_PER_FRAME;
    UINT8 i;
    UINT16 offset;

    if ((UINT16)paint_clear_next_tile + count > PAINT_CANVAS_TILE_COUNT) {
        count = (UINT8)(PAINT_CANVAS_TILE_COUNT - paint_clear_next_tile);
    }

    for (i = 0u; i != count; ++i) {
        offset = paint_tile_offset((UINT8)(paint_clear_next_tile + i));
        memset(&paint_canvas[offset], 0xffu, PAINT_TILE_BYTES);
    }
    paint_upload_tile_range(paint_clear_next_tile, count);
    paint_clear_next_tile = (UINT8)(paint_clear_next_tile + count);

    if (paint_clear_next_tile == PAINT_CANVAS_TILE_COUNT) {
        paint_clear_pending = 0u;
        paint_draw_controls();
    }
}

void paint_enter(void)
{
    ui_scene_begin();

    if (!paint_initialized) {
        memset(paint_canvas, 0xffu, sizeof(paint_canvas));
        paint_primary_shade = 0u;
        paint_secondary_shade = 3u;
        paint_clear_pending = 0u;
        paint_clear_next_tile = 0u;
        paint_initialized = 1u;
    }

    paint_draw_chrome();
    paint_upload_canvas();
    paint_map_canvas();
    paint_draw_swatches();
    if (paint_clear_pending) {
        ui_text_clipped(1u, 15u, "CLEARING CANVAS", PAL_WINDOW, 18u);
    }

    pointer_reset((UINT8)(PAINT_CANVAS_X_PIXELS + PAINT_CANVAS_WIDTH_PIXELS / 2u),
                  (UINT8)(PAINT_CANVAS_Y_PIXELS + PAINT_CANVAS_HEIGHT_PIXELS / 2u));
    pointer_show();
    ui_scene_end();
}

AppState paint_update(const InputState *input)
{
    UINT8 draw_buttons;

    if (input->pressed & J_START) {
        pointer_hide();
        audio_sfx(SFX_CLICK);
        return APP_DESKTOP;
    }

    if (paint_clear_pending) {
        paint_clear_step();
        return APP_PAINT;
    }

    draw_buttons = (UINT8)(input->held & (J_A | J_B));
    if (draw_buttons == (J_A | J_B)) {
        if (input->pressed & (J_A | J_B)) paint_begin_clear();
        return APP_PAINT;
    }

    if (input->pressed & J_SELECT) {
        paint_primary_shade = (UINT8)((paint_primary_shade + 1u) & 3u);
        paint_draw_swatches();
        audio_sfx(SFX_MOVE);
    }

    pointer_update(input);
    if (draw_buttons & J_A) paint_draw_at_pointer(paint_primary_shade);
    else if (draw_buttons & J_B) paint_draw_at_pointer(paint_secondary_shade);

    return APP_PAINT;
}
