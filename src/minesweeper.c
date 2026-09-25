#pragma bank 255

#include <gb/gb.h>
#include <gb/cgb.h>

#include "assets.h"
#include "audio.h"
#include "desktop.h"
#include "minesweeper.h"
#include "ui.h"

#define MS_NO_CELL 0xffu

#define MS_WIN_X 3u
#define MS_WIN_Y 1u
#define MS_WIN_W 14u
#define MS_WIN_H 17u

#define MS_BOARD_PIXEL_X (MS_BOARD_TILE_X * 8u)
#define MS_BOARD_PIXEL_Y (MS_BOARD_TILE_Y * 8u)
#define MS_BOARD_PIXEL_W (MS_MODEL_WIDTH * 8u)
#define MS_BOARD_PIXEL_H (MS_MODEL_HEIGHT * 8u)

#define MS_HEADER_Y 4u
#define MS_COUNTER_X 5u
#define MS_TIMER_X 12u
#define MS_FACE_X 9u

/* Bank-1 art tiles. */
#define MS_ART_HIDDEN 0u
#define MS_ART_FLAG 1u
#define MS_ART_MINE 2u
#define MS_ART_EXPLODED 3u
#define MS_ART_EMPTY 4u
#define MS_ART_NUM_1 5u
#define MS_ART_LED 13u
#define MS_ART_FACE (MS_ART_LED + UI_LED_TILE_COUNT)
#define MS_ART_COUNT (MS_ART_FACE + 16u)

#define MS_FACE_SMILE 0u
#define MS_FACE_PRESS 1u
#define MS_FACE_WIN 2u
#define MS_FACE_LOSE 3u

/* Scene palettes: numbers use two palettes to get the classic colours. */
#define PAL_MS_NUMBERS_A PAL_APP_A
#define PAL_MS_NUMBERS_B PAL_APP_B
#define PAL_MS_FACE PAL_MONO
#define PAL_MS_ALERT PAL_APP_C

static const palette_color_t minesweeper_palettes[] = {
    /* PAL_APP_A: face, green, navy, grid */
    RGB(24, 24, 24), RGB(0, 16, 0), RGB(0, 0, 16), RGB(13, 13, 13),
    /* PAL_APP_B: face, blue, red, grid */
    RGB(24, 24, 24), RGB(0, 0, 31), RGB(31, 0, 0), RGB(13, 13, 13),
    /* PAL_MONO: button face, yellow, highlight, black */
    RGB(24, 24, 24), RGB(31, 31, 0), RGB(31, 31, 31), RGB(0, 0, 0),
    /* PAL_APP_C: face, highlight, red, black (flags, mines, LEDs) */
    RGB(24, 24, 24), RGB(31, 31, 31), RGB(31, 0, 0), RGB(0, 0, 0)
};

/* Colour index and palette per number, following the Windows 3.1 order. */
static const UINT8 number_palette[8] = {
    PAL_MS_NUMBERS_B, PAL_MS_NUMBERS_A, PAL_MS_NUMBERS_B, PAL_MS_NUMBERS_A,
    PAL_MS_NUMBERS_B, PAL_MS_NUMBERS_A, PAL_MS_NUMBERS_B, PAL_MS_NUMBERS_A
};

static const char cell_art[] =
    /* MS_ART_HIDDEN (PAL_WINDOW: raised cell) */
    "00000001" "01111112" "01111112" "01111112"
    "01111112" "01111112" "01111112" "12222222"
    /* MS_ART_FLAG (alert palette) */
    "11111110" "10022003" "10222003" "10022003"
    "10000303" "10003303" "10333333" "03333333"
    /* MS_ART_MINE */
    "00030000" "03333300" "03113300" "33133330"
    "03333300" "03333300" "00030000" "00000000"
    /* MS_ART_EXPLODED */
    "22232222" "23333322" "23113322" "33133332"
    "23333322" "23333322" "22232222" "22222222"
    /* MS_ART_EMPTY (number palettes: grid on top and left) */
    "33333333" "30000000" "30000000" "30000000"
    "30000000" "30000000" "30000000" "30000000";

/* Digits 1-8; ink uses colour 1 or 2 as selected by number_palette. */
static const char number_art[] =
    "33333333" "30001000" "30011000" "30001000"
    "30001000" "30001000" "30011100" "30000000"
    "33333333" "30011000" "30100100" "30000100"
    "30001000" "30010000" "30111100" "30000000"
    "33333333" "30222000" "30000200" "30022000"
    "30000200" "30000200" "30222000" "30000000"
    "33333333" "30002000" "30022000" "30202000"
    "30222200" "30002000" "30002000" "30000000"
    "33333333" "30222200" "30200000" "30222000"
    "30000200" "30000200" "30222000" "30000000"
    "33333333" "30011000" "30100000" "30111000"
    "30100100" "30100100" "30011000" "30000000"
    "33333333" "30333300" "30000300" "30003000"
    "30003000" "30030000" "30030000" "30000000"
    "33333333" "30033000" "30300300" "30033000"
    "30300300" "30300300" "30033000" "30000000";

/* 16x16 faces: 0 button face, 1 yellow, 2 highlight, 3 black. */
static const char face_art[] =
    /* smile */
    "2222222222222223" "2000000000000003" "2000033333300003" "2000311111130003"
    "2003111111113003" "2031113113111303" "2031113113111303" "2031111111111303"
    "2031111111111303" "2031131111311303" "2031113333111303" "2003111111113003"
    "2000311111130003" "2000033333300003" "2000000000000003" "3333333333333333"
    /* pressed: surprised mouth */
    "2222222222222223" "2000000000000003" "2000033333300003" "2000311111130003"
    "2003111111113003" "2031113113111303" "2031113113111303" "2031111111111303"
    "2031111331111303" "2031113113111303" "2031111331111303" "2003111111113003"
    "2000311111130003" "2000033333300003" "2000000000000003" "3333333333333333"
    /* win: sunglasses */
    "2222222222222223" "2000000000000003" "2000033333300003" "2000311111130003"
    "2003111111113003" "2033333333333303" "2031333113331303" "2031131111311303"
    "2031111111111303" "2031131111311303" "2031113333111303" "2003111111113003"
    "2000311111130003" "2000033333300003" "2000000000000003" "3333333333333333"
    /* lose: crossed eyes and frown */
    "2222222222222223" "2000000000000003" "2000033333300003" "2000311111130003"
    "2003131311313003" "2031113113111303" "2031131311313303" "2031111111111303"
    "2031111111111303" "2031113333111303" "2031131111311303" "2003111111113003"
    "2000311111130003" "2000033333300003" "2000000000000003" "3333333333333333";

static MinesweeperModel minesweeper;
static UINT8 focus_x;
static UINT8 focus_y;
static UINT8 exploded_cell;
static UINT8 face_shown;
static UINT8 timer_frames;
static UINT16 timer_seconds;
static UINT8 rendered_tiles[MS_MODEL_CELL_COUNT];
static UINT8 rendered_palettes[MS_MODEL_CELL_COUNT];
static unsigned int next_seed = 0x3101u;

static void load_face_art(void)
{
    UINT8 face;
    UINT8 quadrant;
    UINT8 row;
    UINT8 column;
    const char *source;

    for (face = 0u; face != 4u; ++face) {
        for (quadrant = 0u; quadrant != 4u; ++quadrant) {
            source = &face_art[(UINT16)face * 256u +
                               (UINT16)((quadrant >> 1u) * 8u) * 16u +
                               (UINT16)((quadrant & 1u) * 8u)];
            for (row = 0u; row != 8u; ++row) {
                for (column = 0u; column != 8u; ++column) {
                    ui_art_scratch[(UINT8)(row * 8u + column)] =
                        source[(UINT16)row * 16u + column];
                }
            }
            ui_art_load(1u, (UINT8)(MS_ART_FACE + face * 4u + quadrant), 1u,
                        ui_art_scratch);
        }
    }
}

static UINT8 cell_index(UINT8 x, UINT8 y)
{
    return (UINT8)(y * MS_MODEL_WIDTH + x);
}

static UINT8 focused_cell(void)
{
    return cell_index(focus_x, focus_y);
}

static void cell_look(UINT8 index, UINT8 *tile, UINT8 *palette)
{
    const MinesweeperModelCell *cell = minesweeper_model_get_cell(
        &minesweeper,
        (UINT8)(index % MS_MODEL_WIDTH),
        (UINT8)(index / MS_MODEL_WIDTH)
    );

    *palette = PAL_WINDOW;
    *tile = MS_ART_HIDDEN;
    if (cell == (const MinesweeperModelCell *)0) return;

    if (cell->is_revealed) {
        if (cell->is_mine) {
            *tile = (index == exploded_cell) ? MS_ART_EXPLODED : MS_ART_MINE;
            *palette = PAL_MS_ALERT;
        } else if (cell->adjacent_mines != 0u) {
            *tile = (UINT8)(MS_ART_NUM_1 + cell->adjacent_mines - 1u);
            *palette = number_palette[cell->adjacent_mines - 1u];
        } else {
            *tile = MS_ART_EMPTY;
            *palette = PAL_MS_NUMBERS_B;
        }
    } else if (cell->is_flagged ||
               (minesweeper_model_is_won(&minesweeper) && cell->is_mine)) {
        *tile = MS_ART_FLAG;
        *palette = PAL_MS_ALERT;
    }
}

static void draw_cell(UINT8 index)
{
    UINT8 tile;
    UINT8 palette;

    cell_look(index, &tile, &palette);
    if (rendered_tiles[index] == tile && rendered_palettes[index] == palette) return;
    ui_set_art((UINT8)(MS_BOARD_TILE_X + index % MS_MODEL_WIDTH),
               (UINT8)(MS_BOARD_TILE_Y + index / MS_MODEL_WIDTH),
               tile, palette);
    rendered_tiles[index] = tile;
    rendered_palettes[index] = palette;
}

static void draw_changed_cells(void)
{
    UINT8 index;
    for (index = 0u; index < MS_MODEL_CELL_COUNT; ++index) draw_cell(index);
}

static void invalidate_board(void)
{
    UINT8 index;
    for (index = 0u; index < MS_MODEL_CELL_COUNT; ++index) {
        rendered_tiles[index] = 0xffu;
        rendered_palettes[index] = 0xffu;
    }
}

static void draw_face(UINT8 face)
{
    UINT8 first = (UINT8)(MS_ART_FACE + face * 4u);

    face_shown = face;
    ui_set_art(MS_FACE_X, MS_HEADER_Y, first, PAL_MS_FACE);
    ui_set_art((UINT8)(MS_FACE_X + 1u), MS_HEADER_Y, (UINT8)(first + 1u), PAL_MS_FACE);
    ui_set_art(MS_FACE_X, (UINT8)(MS_HEADER_Y + 1u), (UINT8)(first + 2u), PAL_MS_FACE);
    ui_set_art((UINT8)(MS_FACE_X + 1u), (UINT8)(MS_HEADER_Y + 1u),
               (UINT8)(first + 3u), PAL_MS_FACE);
}

static UINT8 resting_face(void)
{
    if (minesweeper_model_is_won(&minesweeper)) return MS_FACE_WIN;
    if (minesweeper_model_is_lost(&minesweeper)) return MS_FACE_LOSE;
    return MS_FACE_SMILE;
}

static void draw_counters(void)
{
    UINT8 flags = minesweeper_model_is_won(&minesweeper) ?
        MS_MODEL_MINE_COUNT : minesweeper_model_get_flag_count(&minesweeper);

    ui_led_draw(MS_COUNTER_X, MS_HEADER_Y, MS_ART_LED,
                (UINT16)(MS_MODEL_MINE_COUNT - flags), 3u, PAL_MS_ALERT);
    ui_led_draw(MS_TIMER_X, MS_HEADER_Y, MS_ART_LED, timer_seconds, 3u, PAL_MS_ALERT);
}

static void move_pointer_to_focus(void)
{
    pointer_move_to((UINT8)((MS_BOARD_TILE_X + focus_x) * 8u + 3u),
                    (UINT8)((MS_BOARD_TILE_Y + focus_y) * 8u + 3u));
}

static UINT8 hovered_cell(void)
{
    const PointerState *pointer;

    if (!pointer_hits(MS_BOARD_PIXEL_X, MS_BOARD_PIXEL_Y,
                      MS_BOARD_PIXEL_W, MS_BOARD_PIXEL_H)) return MS_NO_CELL;
    pointer = pointer_get();
    return cell_index((UINT8)((pointer->x - MS_BOARD_PIXEL_X) >> 3u),
                      (UINT8)((pointer->y - MS_BOARD_PIXEL_Y) >> 3u));
}

static void start_new_board(void)
{
    minesweeper_model_init(&minesweeper, next_seed);
    next_seed = (unsigned int)((next_seed + 0x9e37u) & 65535u);
    exploded_cell = MS_NO_CELL;
    timer_frames = 0u;
    timer_seconds = 0u;
    focus_x = (UINT8)(MS_MODEL_WIDTH / 2u);
    focus_y = (UINT8)(MS_MODEL_HEIGHT / 2u);
    move_pointer_to_focus();
    draw_changed_cells();
    draw_counters();
    draw_face(MS_FACE_SMILE);
}

static void after_action(UINT8 action)
{
    if (action == MS_MODEL_ACTION_NONE) {
        audio_sfx(SFX_ERROR);
        return;
    }
    if (action == MS_MODEL_ACTION_LOST) audio_sfx(SFX_ERROR);
    else if (action == MS_MODEL_ACTION_WON) audio_sfx(SFX_WIN);
    else audio_sfx(SFX_CLICK);
    draw_changed_cells();
    draw_counters();
    draw_face(resting_face());
}

static void tick_timer(void)
{
    if (!minesweeper_model_are_mines_placed(&minesweeper) ||
        minesweeper_model_get_status(&minesweeper) != MS_MODEL_PLAYING ||
        timer_seconds >= 999u) return;
    if (++timer_frames < 60u) return;
    timer_frames = 0u;
    ++timer_seconds;
    ui_led_draw(MS_TIMER_X, MS_HEADER_Y, MS_ART_LED, timer_seconds, 3u, PAL_MS_ALERT);
}

void minesweeper_enter(void) BANKED
{
    ui_scene_begin();
    set_bkg_palette(PAL_APP_A, 1u, &minesweeper_palettes[0]);
    set_bkg_palette(PAL_APP_B, 1u, &minesweeper_palettes[4]);
    set_bkg_palette(PAL_MONO, 1u, &minesweeper_palettes[8]);
    set_bkg_palette(PAL_APP_C, 1u, &minesweeper_palettes[12]);
    ui_art_load(1u, MS_ART_HIDDEN, 5u, cell_art);
    ui_art_load(1u, MS_ART_NUM_1, 8u, number_art);
    ui_led_load(MS_ART_LED);
    load_face_art();

    desktop_draw_backdrop();
    ui_window(MS_WIN_X, MS_WIN_Y, MS_WIN_W, MS_WIN_H, "Sweeper", 1u, UI_CLIENT_FACE);
    ui_menu((UINT8)(MS_WIN_X + 1u), (UINT8)(MS_WIN_Y + 1u),
            (UINT8)(MS_WIN_W - 2u), "Game  Help");
    ui_sunken(MS_COUNTER_X, MS_HEADER_Y, MS_MODEL_WIDTH, 2u);
    ui_sunken(MS_BOARD_TILE_X, MS_BOARD_TILE_Y, MS_MODEL_WIDTH, MS_MODEL_HEIGHT);

    invalidate_board();
    pointer_reset(80u, 72u);
    start_new_board();
    pointer_show();
    ui_scene_end();
}

AppState minesweeper_update(const InputState *input) BANKED
{
    UINT8 hover;
    UINT8 next_focus;
    UINT8 face;

    if ((input->pressed & J_START) ||
        ((input->pressed & J_A) &&
         pointer_in_tiles(MS_WIN_X, MS_WIN_Y, 2u, 1u))) {
        audio_sfx(SFX_CLICK);
        pointer_hide();
        return APP_DESKTOP;
    }

    pointer_update(input);
    tick_timer();
    hover = hovered_cell();
    if (hover != MS_NO_CELL && hover != focused_cell()) {
        focus_x = (UINT8)(hover % MS_MODEL_WIDTH);
        focus_y = (UINT8)(hover / MS_MODEL_WIDTH);
    }

    if (input->pressed & J_SELECT) {
        next_focus = (UINT8)(focused_cell() + 1u);
        if (next_focus >= MS_MODEL_CELL_COUNT) next_focus = 0u;
        focus_x = (UINT8)(next_focus % MS_MODEL_WIDTH);
        focus_y = (UINT8)(next_focus / MS_MODEL_WIDTH);
        move_pointer_to_focus();
        hover = next_focus;
        audio_sfx(SFX_MOVE);
    }

    if (input->pressed & J_A) {
        if (pointer_in_tiles(MS_FACE_X, MS_HEADER_Y, 2u, 2u) ||
            pointer_hits((UINT8)((MS_WIN_X + 1u) << 3u), (UINT8)((MS_WIN_Y + 1u) << 3u),
                         24u, 8u)) {
            start_new_board();
            audio_sfx(SFX_CLICK);
            return APP_SWEEPER;
        }
        if (hover != MS_NO_CELL) {
            face = minesweeper_model_reveal(&minesweeper, focus_x, focus_y);
            if (face == MS_MODEL_ACTION_LOST) exploded_cell = focused_cell();
            after_action(face);
        } else {
            audio_sfx(SFX_ERROR);
        }
    }

    if ((input->pressed & J_B) && hover != MS_NO_CELL) {
        after_action(minesweeper_model_toggle_flag(&minesweeper, focus_x, focus_y));
    }

    face = resting_face();
    if (face == MS_FACE_SMILE && (input->held & J_A) && hover != MS_NO_CELL) {
        face = MS_FACE_PRESS;
    }
    if (face != face_shown) draw_face(face);
    return APP_SWEEPER;
}
