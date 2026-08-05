#include <gb/gb.h>

#include "assets.h"
#include "audio.h"
#include "minesweeper.h"
#include "ui.h"

#define MS_NO_CELL 0xffu

#define MS_BOARD_PIXEL_X (MS_BOARD_TILE_X * 8u)
#define MS_BOARD_PIXEL_Y (MS_BOARD_TILE_Y * 8u)
#define MS_BOARD_PIXEL_W (MS_MODEL_WIDTH * 8u)
#define MS_BOARD_PIXEL_H (MS_MODEL_HEIGHT * 8u)

#define MS_SYSTEM_PIXEL_X 8u
#define MS_SYSTEM_PIXEL_Y 0u
#define MS_SYSTEM_PIXEL_W 8u
#define MS_SYSTEM_PIXEL_H 8u

#define MS_GAME_MENU_PIXEL_X 8u
#define MS_GAME_MENU_PIXEL_Y 8u
#define MS_GAME_MENU_PIXEL_W 32u
#define MS_GAME_MENU_PIXEL_H 8u

#define MS_HELP_MENU_PIXEL_X 56u
#define MS_HELP_MENU_PIXEL_Y 8u
#define MS_HELP_MENU_PIXEL_W 32u
#define MS_HELP_MENU_PIXEL_H 8u

static MinesweeperModel minesweeper;
static UINT8 focus_x;
static UINT8 focus_y;
static UINT8 exploded_cell;
static UINT8 help_visible;
static UINT8 rendered_tiles[MS_MODEL_CELL_COUNT];
static UINT8 rendered_palettes[MS_MODEL_CELL_COUNT];
static unsigned int next_seed = 0x3101u;

static UINT8 cell_index(UINT8 x, UINT8 y)
{
    return (UINT8)(y * MS_MODEL_WIDTH + x);
}

static UINT8 focused_cell(void)
{
    return cell_index(focus_x, focus_y);
}

static UINT8 tile_for_cell(UINT8 index)
{
    const MinesweeperModelCell *cell;

    cell = minesweeper_model_get_cell(
        &minesweeper,
        (UINT8)(index % MS_MODEL_WIDTH),
        (UINT8)(index / MS_MODEL_WIDTH)
    );
    if (cell == (const MinesweeperModelCell *)0) return TILE_BLANK;

    if (cell->is_revealed) {
        if (cell->is_mine) {
            return (index == exploded_cell) ? TILE_MS_EXPLODED : TILE_MS_MINE;
        }
        if (cell->adjacent_mines != 0u) {
            return (UINT8)(TILE_MS_NUM_1 + cell->adjacent_mines - 1u);
        }
        return TILE_BLANK;
    }

    if (cell->is_flagged ||
        (minesweeper_model_is_won(&minesweeper) && cell->is_mine)) {
        return TILE_MS_FLAG;
    }
    return TILE_MS_HIDDEN;
}

static UINT8 palette_for_cell(UINT8 index)
{
    return (index == focused_cell()) ? PAL_GAME_FOCUS : PAL_GAME;
}

static void draw_cell(UINT8 index)
{
    UINT8 tile;
    UINT8 palette;
    UINT8 x;
    UINT8 y;

    tile = tile_for_cell(index);
    palette = palette_for_cell(index);
    if (rendered_tiles[index] == tile &&
        rendered_palettes[index] == palette) return;

    x = (UINT8)(index % MS_MODEL_WIDTH);
    y = (UINT8)(index / MS_MODEL_WIDTH);
    ui_set_tile(
        (UINT8)(MS_BOARD_TILE_X + x),
        (UINT8)(MS_BOARD_TILE_Y + y),
        tile,
        palette
    );
    rendered_tiles[index] = tile;
    rendered_palettes[index] = palette;
}

static void draw_changed_cells(void)
{
    UINT8 index;

    for (index = 0u; index < MS_MODEL_CELL_COUNT; ++index) {
        draw_cell(index);
    }
}

static void invalidate_board(void)
{
    UINT8 index;

    for (index = 0u; index < MS_MODEL_CELL_COUNT; ++index) {
        rendered_tiles[index] = 0xffu;
        rendered_palettes[index] = 0xffu;
    }
}

static void set_two_digits(char *text, UINT8 offset, UINT8 value)
{
    text[offset] = (char)('0' + (value / 10u));
    text[(UINT8)(offset + 1u)] = (char)('0' + (value % 10u));
}

static void draw_status(void)
{
    char status_text[] = "MINES:00 FLAGS:00";
    UINT8 flags;
    UINT8 remaining;

    if (minesweeper_model_is_won(&minesweeper)) {
        flags = MS_MODEL_MINE_COUNT;
        remaining = 0u;
    } else {
        flags = minesweeper_model_get_flag_count(&minesweeper);
        remaining = (UINT8)(MS_MODEL_MINE_COUNT - flags);
    }

    set_two_digits(status_text, 6u, remaining);
    set_two_digits(status_text, 15u, flags);
    ui_text_clipped(1u, 3u, status_text, PAL_WINDOW, 18u);
}

static void draw_help(void)
{
    const char *message;

    if (minesweeper_model_is_won(&minesweeper)) {
        message = "YOU WIN!";
    } else if (minesweeper_model_is_lost(&minesweeper)) {
        message = "GAME OVER";
    } else if (help_visible) {
        message = "SELECT:NEXT CELL";
    } else {
        message = "CLEAR ALL SAFE TILES";
    }

    ui_text_clipped(1u, 14u, message, PAL_WINDOW, 18u);
    ui_text_clipped(1u, 15u, "A:OPEN  B:FLAG", PAL_WINDOW, 18u);
    ui_text_clipped(1u, 16u, "ST:EXIT SEL:NEXT", PAL_WINDOW, 18u);
}

static void move_pointer_to_focus(void)
{
    pointer_move_to(
        (UINT8)((MS_BOARD_TILE_X + focus_x) * 8u + 3u),
        (UINT8)((MS_BOARD_TILE_Y + focus_y) * 8u + 3u)
    );
}

static void set_focus(UINT8 index, UINT8 move_pointer)
{
    UINT8 previous;

    if (index >= MS_MODEL_CELL_COUNT) return;
    previous = focused_cell();
    focus_x = (UINT8)(index % MS_MODEL_WIDTH);
    focus_y = (UINT8)(index / MS_MODEL_WIDTH);

    if (previous != index) {
        draw_cell(previous);
        draw_cell(index);
    }
    if (move_pointer) move_pointer_to_focus();
}

static UINT8 hovered_cell(void)
{
    const PointerState *pointer;
    UINT8 x;
    UINT8 y;

    if (!pointer_hits(
            MS_BOARD_PIXEL_X,
            MS_BOARD_PIXEL_Y,
            MS_BOARD_PIXEL_W,
            MS_BOARD_PIXEL_H
        )) return MS_NO_CELL;

    pointer = pointer_get();
    x = (UINT8)((pointer->x - MS_BOARD_PIXEL_X) / 8u);
    y = (UINT8)((pointer->y - MS_BOARD_PIXEL_Y) / 8u);
    return cell_index(x, y);
}

static void start_new_board(void)
{
    minesweeper_model_init(&minesweeper, next_seed);
    next_seed = (unsigned int)((next_seed + 0x9e37u) & 65535u);
    exploded_cell = MS_NO_CELL;
    help_visible = 0u;
    focus_x = (UINT8)(MS_MODEL_WIDTH / 2u);
    focus_y = (UINT8)(MS_MODEL_HEIGHT / 2u);
    move_pointer_to_focus();
    draw_changed_cells();
    draw_status();
    draw_help();
}

static void apply_reveal(void)
{
    UINT8 action;
    UINT8 index;

    index = focused_cell();
    action = minesweeper_model_reveal(&minesweeper, focus_x, focus_y);
    if (action == MS_MODEL_ACTION_NONE) {
        audio_sfx(SFX_ERROR);
        return;
    }

    help_visible = 0u;
    if (action == MS_MODEL_ACTION_LOST) {
        exploded_cell = index;
        audio_sfx(SFX_ERROR);
    } else if (action == MS_MODEL_ACTION_WON) {
        audio_sfx(SFX_WIN);
    } else {
        audio_sfx(SFX_CLICK);
    }
    draw_changed_cells();
    draw_status();
    draw_help();
}

static void apply_flag(void)
{
    UINT8 action;

    action = minesweeper_model_toggle_flag(
        &minesweeper,
        focus_x,
        focus_y
    );
    if (action == MS_MODEL_ACTION_NONE) {
        audio_sfx(SFX_ERROR);
        return;
    }

    help_visible = 0u;
    audio_sfx(SFX_CLICK);
    draw_changed_cells();
    draw_status();
    draw_help();
}

void minesweeper_enter(void)
{
    ui_scene_begin();
    ui_clear(PAL_DESKTOP);
    ui_window(0u, 0u, 20u, 18u, "GB SWEEPER", 1u);
    ui_menu(1u, 1u, 18u, "GAME  HELP");
    invalidate_board();
    pointer_reset(80u, 72u);
    start_new_board();
    pointer_show();
    ui_scene_end();
}

AppState minesweeper_update(const InputState *input)
{
    UINT8 hover;
    UINT8 next_focus;

    if (input->pressed & J_START) {
        audio_sfx(SFX_CLICK);
        pointer_hide();
        return APP_DESKTOP;
    }

    pointer_update(input);
    hover = hovered_cell();
    if (hover != MS_NO_CELL && hover != focused_cell()) {
        set_focus(hover, 0u);
    }

    if (input->pressed & J_SELECT) {
        next_focus = (UINT8)(focused_cell() + 1u);
        if (next_focus >= MS_MODEL_CELL_COUNT) next_focus = 0u;
        set_focus(next_focus, 1u);
        hover = next_focus;
        audio_sfx(SFX_MOVE);
    }

    if (input->pressed & J_A) {
        if (pointer_hits(
                MS_SYSTEM_PIXEL_X,
                MS_SYSTEM_PIXEL_Y,
                MS_SYSTEM_PIXEL_W,
                MS_SYSTEM_PIXEL_H
            )) {
            audio_sfx(SFX_CLICK);
            pointer_hide();
            return APP_DESKTOP;
        }
        if (pointer_hits(
                MS_GAME_MENU_PIXEL_X,
                MS_GAME_MENU_PIXEL_Y,
                MS_GAME_MENU_PIXEL_W,
                MS_GAME_MENU_PIXEL_H
            )) {
            start_new_board();
            audio_sfx(SFX_CLICK);
            return APP_SWEEPER;
        }
        if (pointer_hits(
                MS_HELP_MENU_PIXEL_X,
                MS_HELP_MENU_PIXEL_Y,
                MS_HELP_MENU_PIXEL_W,
                MS_HELP_MENU_PIXEL_H
            )) {
            help_visible = (UINT8)!help_visible;
            draw_help();
            audio_sfx(SFX_CLICK);
            return APP_SWEEPER;
        }
        if (hover != MS_NO_CELL) {
            apply_reveal();
        } else {
            audio_sfx(SFX_ERROR);
        }
    }

    if (input->pressed & J_B) apply_flag();
    return APP_SWEEPER;
}
