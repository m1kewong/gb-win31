#pragma bank 255

#include <gb/gb.h>
#include <gb/cgb.h>

#include "assets.h"
#include "audio.h"
#include "solitaire.h"
#include "text.h"
#include "ui.h"

/* Layout in tiles. Cards are 2x2 tiles; columns sit on a 2-tile pitch. */
#define SOL_TOP_Y 2u
#define SOL_TABLEAU_Y 5u
#define SOL_TABLEAU_BOTTOM 15u
#define SOL_TABLEAU_ROWS (SOL_TABLEAU_BOTTOM - SOL_TABLEAU_Y + 1u)
#define SOL_COLUMN_X(column) ((UINT8)(3u + ((column) << 1u)))
#define SOL_WASTE_X 5u
#define SOL_STATUS_Y 16u
#define SOL_WASTE_FAN_MAX 3u

#define SOL_ROW_MENU 0u
#define SOL_ROW_TOP 1u
#define SOL_ROW_TABLEAU 2u
#define SOL_EMPTY_COLUMN 2u

#define PAL_CARD_BLACK PAL_APP_A
#define PAL_CARD_RED PAL_APP_B
#define PAL_CARD_BACK PAL_MONO
#define PAL_CARD_SELECT PAL_APP_C

/* Colour 0 is the felt in every card palette. */
static const palette_color_t solitaire_palettes[] = {
    RGB(0, 17, 4), RGB(31, 31, 31), RGB(0, 0, 0), RGB(0, 0, 0),
    RGB(0, 17, 4), RGB(31, 31, 31), RGB(29, 0, 0), RGB(0, 0, 0),
    RGB(0, 17, 4), RGB(31, 31, 31), RGB(2, 8, 26), RGB(0, 0, 0),
    RGB(0, 17, 4), RGB(0, 0, 14), RGB(31, 31, 31), RGB(0, 28, 31)
};

/* Bank-1 art tiles. */
#define ART_RANK 0u
#define ART_SUIT 13u
#define ART_BODY_L 17u
#define ART_BODY_R 21u
#define ART_BACK_TOP 25u
#define ART_BACK_DOUBLE 27u
#define ART_BACK_BOTTOM 29u
#define ART_EMPTY_TOP 31u
#define ART_EMPTY_BOTTOM 33u
#define ART_RECYCLE_TOP 35u

static const char rank_glyphs[13][5][6] = {
    {".#...", "#.#..", "###..", "#.#..", "#.#.."},
    {"##...", "..#..", ".#...", "#....", "###.."},
    {"##...", "..#..", ".#...", "..#..", "##..."},
    {"#.#..", "#.#..", "###..", "..#..", "..#.."},
    {"###..", "#....", "##...", "..#..", "##..."},
    {".##..", "#....", "##...", "#.#..", ".#..."},
    {"###..", "..#..", ".#...", ".#...", ".#..."},
    {".#...", "#.#..", ".#...", "#.#..", ".#..."},
    {".#...", "#.#..", ".##..", "..#..", "##..."},
    {"#..#.", "#.#.#", "#.#.#", "#.#.#", "#..#."},
    {"..#..", "..#..", "..#..", "#.#..", ".#..."},
    {".##..", "#..#.", "#..#.", "#.#..", ".#.#."},
    {"#.#..", "#.#..", "##...", "#.#..", "#.#.."}
};

/* Clubs, diamonds, hearts, spades. */
static const char suit_glyphs[4][5][6] = {
    {".###.", ".###.", "#####", "#.#.#", "..#.."},
    {"..#..", ".###.", "#####", ".###.", "..#.."},
    {".#.#.", "#####", "#####", ".###.", "..#.."},
    {"..#..", ".###.", "#####", "##.##", "..#.."}
};

static const char pip_glyphs[4][6][8] = {
    {"..###..", "..###..", "#######", "###.###", "...#...", "..###.."},
    {"...#...", "..###..", ".#####.", ".#####.", "..###..", "...#..."},
    {".##.##.", "#######", "#######", ".#####.", "..###..", "...#..."},
    {"...#...", "..###..", ".#####.", "#######", "##.#.##", "..###.."}
};

static const char ring_glyph[6][7] = {
    ".####.", "#....#", "#....#", "#....#", "#....#", ".####."
};

/* Exported so the emulator smoke test can inspect the game state. */
SolitaireModel solitaire;

static UINT8 cursor_row;
static UINT8 cursor_col;
static UINT8 cursor_depth;
static UINT8 selected_pile;
static UINT8 selected_count;
static UINT8 autoplay;
static UINT8 autoplay_wait;
static UINT8 timer_running;
static UINT8 timer_frames;
static UINT16 timer_seconds;
static UINT8 column_face_row[SOL_TABLEAU_COUNT];
static UINT8 column_face_index[SOL_TABLEAU_COUNT];
static unsigned int next_seed = 0x5701u;
static UiLabel score_label;
static UiLabel time_label;
static char status_text[20];

/* --- art ---------------------------------------------------------------- */

static void art_card(UINT8 half, UINT8 top_edge, UINT8 bottom_edge, char face)
{
    UINT8 row;
    UINT8 column;
    UINT8 card_x;
    char pixel;

    for (row = 0u; row != 8u; ++row) {
        for (column = 0u; column != 8u; ++column) {
            card_x = (UINT8)((half << 3u) + column);
            pixel = face;
            if (card_x == 0u || card_x == 15u) pixel = '0';
            else if (card_x == 1u || card_x == 14u) pixel = '3';
            if ((top_edge && row == 0u) || (bottom_edge && row == 7u)) {
                pixel = (card_x <= 1u || card_x >= 14u) ? '0' : '3';
            }
            if (face == 'P' && pixel == 'P') {
                pixel = (((UINT8)(card_x + row)) & 3u) ? '2' : '1';
            }
            ui_art_scratch[(UINT8)((row << 3u) + column)] = pixel;
        }
    }
}

static void art_glyph(UINT8 x, UINT8 y, const char *rows, UINT8 height, UINT8 stride)
{
    UINT8 row;
    UINT8 column;

    for (row = 0u; row != height; ++row) {
        for (column = 0u; rows[(UINT8)(row * stride + column)] != '\0'; ++column) {
            if (rows[(UINT8)(row * stride + column)] == '#' &&
                (UINT8)(x + column) < 8u && (UINT8)(y + row) < 8u) {
                ui_art_scratch[(UINT8)(((UINT8)(y + row) << 3u) + x + column)] = '2';
            }
        }
    }
}

static void art_pip(UINT8 half, UINT8 suit)
{
    UINT8 row;
    UINT8 column;
    UINT8 card_x;

    for (row = 0u; row != 6u; ++row) {
        for (column = 0u; column != 8u; ++column) {
            card_x = (UINT8)((half << 3u) + column);
            if (card_x < 4u || card_x > 10u) continue;
            if (pip_glyphs[suit][row][card_x - 4u] == '#') {
                ui_art_scratch[(UINT8)((row << 3u) + column)] = '2';
            }
        }
    }
}

static void load_art(UINT8 tile)
{
    ui_art_load(1u, tile, 1u, ui_art_scratch);
}

static void build_card_art(void)
{
    UINT8 index;
    UINT8 half;

    for (index = 0u; index != SOL_RANKS; ++index) {
        art_card(0u, 1u, 0u, '1');
        art_glyph(3u, 2u, &rank_glyphs[index][0][0], 5u, 6u);
        load_art((UINT8)(ART_RANK + index));
    }
    for (index = 0u; index != 4u; ++index) {
        art_card(1u, 1u, 0u, '1');
        art_glyph(1u, 2u, &suit_glyphs[index][0][0], 5u, 6u);
        load_art((UINT8)(ART_SUIT + index));
        for (half = 0u; half != 2u; ++half) {
            art_card(half, 0u, 1u, '1');
            art_pip(half, index);
            load_art((UINT8)((half ? ART_BODY_R : ART_BODY_L) + index));
        }
    }
    for (half = 0u; half != 2u; ++half) {
        art_card(half, 1u, 0u, 'P');
        load_art((UINT8)(ART_BACK_TOP + half));
        art_card(half, 1u, 0u, 'P');
        for (index = 0u; index != 8u; ++index) ui_art_scratch[(UINT8)(32u + index)] = ui_art_scratch[index];
        load_art((UINT8)(ART_BACK_DOUBLE + half));
        art_card(half, 0u, 1u, 'P');
        load_art((UINT8)(ART_BACK_BOTTOM + half));
        art_card(half, 1u, 0u, '0');
        load_art((UINT8)(ART_EMPTY_TOP + half));
        art_card(half, 1u, 0u, '0');
        art_glyph(half ? 0u : 5u, 2u, half ? &ring_glyph[0][3] : &ring_glyph[0][0], 6u, 7u);
        load_art((UINT8)(ART_RECYCLE_TOP + half));
        art_card(half, 0u, 1u, '0');
        load_art((UINT8)(ART_EMPTY_BOTTOM + half));
    }
}

/* --- drawing ------------------------------------------------------------ */

static UINT8 card_palette(UINT8 pile, UINT8 index, UINT8 card)
{
    if (pile == selected_pile &&
        index >= (UINT8)(solitaire_model_pile_size(&solitaire, pile) - selected_count)) {
        return PAL_CARD_SELECT;
    }
    return SOL_IS_RED(card) ? PAL_CARD_RED : PAL_CARD_BLACK;
}

static void put_pair(UINT8 x, UINT8 y, UINT8 left, UINT8 right, UINT8 palette)
{
    ui_set_art(x, y, left, palette);
    ui_set_art((UINT8)(x + 1u), y, right, palette);
}

static void put_felt(UINT8 x, UINT8 y, UINT8 width)
{
    ui_fill(x, y, width, 1u, TILE_BLANK, PAL_CARD_BLACK);
}

static void put_strip(UINT8 x, UINT8 y, UINT8 card, UINT8 palette)
{
    put_pair(x, y, (UINT8)(ART_RANK + SOL_RANK(card)), (UINT8)(ART_SUIT + SOL_SUIT(card)), palette);
}

static void put_card(UINT8 x, UINT8 y, UINT8 card, UINT8 palette)
{
    put_strip(x, y, card, palette);
    put_pair(x, (UINT8)(y + 1u), (UINT8)(ART_BODY_L + SOL_SUIT(card)),
             (UINT8)(ART_BODY_R + SOL_SUIT(card)), palette);
}

static void put_outline(UINT8 x, UINT8 y, UINT8 top_tile)
{
    put_pair(x, y, top_tile, (UINT8)(top_tile + 1u), PAL_CARD_BLACK);
    put_pair(x, (UINT8)(y + 1u), ART_EMPTY_BOTTOM, ART_EMPTY_BOTTOM + 1u, PAL_CARD_BLACK);
}

static void draw_stock(void)
{
    UINT8 x = SOL_COLUMN_X(0u);

    if (solitaire.stock_count != 0u) {
        put_pair(x, SOL_TOP_Y, ART_BACK_TOP, ART_BACK_TOP + 1u, PAL_CARD_BACK);
        put_pair(x, SOL_TOP_Y + 1u, ART_BACK_BOTTOM, ART_BACK_BOTTOM + 1u, PAL_CARD_BACK);
    } else {
        put_outline(x, SOL_TOP_Y, solitaire.waste_count ? ART_RECYCLE_TOP : ART_EMPTY_TOP);
    }
}

static void draw_waste(void)
{
    UINT8 count = solitaire.waste_count;
    UINT8 show = (solitaire.draw_count == 3u) ? SOL_WASTE_FAN_MAX : 1u;
    UINT8 i;
    UINT8 index;
    UINT8 card;
    UINT8 palette;
    UINT8 x;

    if (show > count) show = count;
    put_felt(SOL_WASTE_X, SOL_TOP_Y, 4u);
    put_felt(SOL_WASTE_X, SOL_TOP_Y + 1u, 4u);
    for (i = 0u; i != show; ++i) {
        index = (UINT8)(count - show + i);
        card = solitaire.waste[index];
        palette = card_palette(SOL_PILE_WASTE, index, card);
        x = (UINT8)(SOL_WASTE_X + i);
        if (i + 1u == show) {
            put_card(x, SOL_TOP_Y, card, palette);
        } else {
            ui_set_art(x, SOL_TOP_Y, (UINT8)(ART_RANK + SOL_RANK(card)), palette);
            ui_set_art(x, SOL_TOP_Y + 1u, (UINT8)(ART_BODY_L + SOL_SUIT(card)), palette);
        }
    }
}

static void draw_foundation(UINT8 foundation)
{
    UINT8 pile = (UINT8)(SOL_PILE_FOUNDATION + foundation);
    UINT8 card = solitaire_model_top(&solitaire, pile);
    UINT8 x = SOL_COLUMN_X((UINT8)(3u + foundation));

    if (card == SOL_NO_CARD) {
        put_outline(x, SOL_TOP_Y, ART_EMPTY_TOP);
    } else {
        put_card(x, SOL_TOP_Y, card,
                 card_palette(pile, (UINT8)(solitaire_model_pile_size(&solitaire, pile) - 1u), card));
    }
}

static void draw_column(UINT8 column)
{
    UINT8 pile = (UINT8)(SOL_PILE_TABLEAU + column);
    UINT8 x = SOL_COLUMN_X(column);
    UINT8 y = SOL_TABLEAU_Y;
    UINT8 count = solitaire.tableau_count[column];
    UINT8 hidden = solitaire.tableau_hidden[column];
    UINT8 back_tiles = (UINT8)((hidden + 1u) >> 1u);
    UINT8 first;
    UINT8 index;
    UINT8 card;
    UINT8 i;

    if (count == 0u) {
        put_outline(x, y, ART_EMPTY_TOP);
        column_face_row[column] = y;
        column_face_index[column] = 0u;
        y = (UINT8)(y + 2u);
    } else {
        /* Squeeze long columns: first fold the backs, then hide old strips. */
        if ((UINT8)(back_tiles + count - hidden + 1u) > SOL_TABLEAU_ROWS && back_tiles > 1u) {
            back_tiles = 1u;
        }
        first = hidden;
        if ((UINT8)(back_tiles + count - hidden + 1u) > SOL_TABLEAU_ROWS) {
            first = (UINT8)(count + back_tiles + 1u - SOL_TABLEAU_ROWS);
        }
        for (i = 0u; i != back_tiles; ++i) {
            if ((back_tiles == 1u && hidden > 1u) || (UINT8)((i << 1u) + 1u) < hidden) {
                put_pair(x, y, ART_BACK_DOUBLE, ART_BACK_DOUBLE + 1u, PAL_CARD_BACK);
            } else {
                put_pair(x, y, ART_BACK_TOP, ART_BACK_TOP + 1u, PAL_CARD_BACK);
            }
            ++y;
        }
        column_face_row[column] = y;
        column_face_index[column] = first;
        for (index = first; index != (UINT8)(count - 1u); ++index) {
            card = solitaire.tableau[column][index];
            put_strip(x, y, card, card_palette(pile, index, card));
            ++y;
        }
        card = solitaire.tableau[column][count - 1u];
        put_card(x, y, card, card_palette(pile, (UINT8)(count - 1u), card));
        y = (UINT8)(y + 2u);
    }

    while (y <= SOL_TABLEAU_BOTTOM) {
        put_felt(x, y, 2u);
        ++y;
    }
}

static void draw_pile(UINT8 pile)
{
    if (pile == SOL_PILE_STOCK) draw_stock();
    else if (pile == SOL_PILE_WASTE) draw_waste();
    else if (pile < SOL_PILE_TABLEAU) draw_foundation((UINT8)(pile - SOL_PILE_FOUNDATION));
    else if (pile < SOL_PILE_COUNT) draw_column((UINT8)(pile - SOL_PILE_TABLEAU));
}

static void draw_table(void)
{
    UINT8 pile;

    for (pile = 0u; pile != SOL_PILE_COUNT; ++pile) draw_pile(pile);
}

static void write_number(char *text, UINT16 value)
{
    char digits[6];
    UINT8 count = 0u;

    do {
        digits[count++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value != 0u);
    while (count != 0u) *text++ = digits[--count];
    *text = '\0';
}

static void draw_score(void)
{
    UINT8 colors = TEXT_COLORS(COLOR_GREY, COLOR_BLACK, 0u);

    if (solitaire_model_is_won(&solitaire)) {
        ui_label_set(&score_label, "You won!", colors, 0u);
        return;
    }
    status_text[0] = 'S'; status_text[1] = 'c'; status_text[2] = 'o';
    status_text[3] = 'r'; status_text[4] = 'e'; status_text[5] = ':';
    status_text[6] = ' ';
    write_number(&status_text[7], (UINT16)solitaire_model_score(&solitaire));
    ui_label_set(&score_label, status_text, colors, 0u);
}

static void draw_time(void)
{
    UINT8 colors = TEXT_COLORS(COLOR_GREY, COLOR_BLACK, 0u);

    if (solitaire_model_is_won(&solitaire)) {
        ui_label_set(&time_label, "A: deal again", colors, TEXT_ALIGN_RIGHT);
        return;
    }
    status_text[0] = 'T'; status_text[1] = 'i'; status_text[2] = 'm';
    status_text[3] = 'e'; status_text[4] = ':'; status_text[5] = ' ';
    write_number(&status_text[6], timer_seconds);
    ui_label_set(&time_label, status_text, colors, TEXT_ALIGN_RIGHT);
}

static void draw_status(void)
{
    draw_score();
    draw_time();
}

/* --- cursor ------------------------------------------------------------- */

static UINT8 cursor_pile(void)
{
    if (cursor_row == SOL_ROW_TABLEAU) return (UINT8)(SOL_PILE_TABLEAU + cursor_col);
    if (cursor_row != SOL_ROW_TOP) return SOL_NO_PILE;
    if (cursor_col == 0u) return SOL_PILE_STOCK;
    if (cursor_col == 1u) return SOL_PILE_WASTE;
    if (cursor_col >= 3u) return (UINT8)(SOL_PILE_FOUNDATION + cursor_col - 3u);
    return SOL_NO_PILE;
}

static UINT8 face_up_count(UINT8 column)
{
    return (UINT8)(solitaire.tableau_count[column] - solitaire.tableau_hidden[column]);
}

static void place_pointer(void)
{
    UINT8 x = (UINT8)((SOL_COLUMN_X(cursor_col) << 3u) + 8u);
    UINT8 y;
    UINT8 count;
    UINT8 index;

    if (cursor_row == SOL_ROW_MENU) {
        pointer_move_to(14u, 11u);
        return;
    }
    if (cursor_row == SOL_ROW_TOP) {
        if (cursor_col == 1u && solitaire.draw_count == 3u && solitaire.waste_count > 1u) {
            index = (solitaire.waste_count >= SOL_WASTE_FAN_MAX) ? SOL_WASTE_FAN_MAX : solitaire.waste_count;
            x = (UINT8)(((SOL_WASTE_X + index - 1u) << 3u) + 8u);
        }
        pointer_move_to(x, (UINT8)((SOL_TOP_Y << 3u) + 5u));
        return;
    }

    count = solitaire.tableau_count[cursor_col];
    y = column_face_row[cursor_col];
    if (count != 0u) {
        index = (UINT8)(count - cursor_depth);
        if (index > column_face_index[cursor_col]) {
            y = (UINT8)(y + index - column_face_index[cursor_col]);
        }
    }
    pointer_move_to(x, (UINT8)((y << 3u) + 3u));
}

static void clamp_depth(void)
{
    UINT8 limit = face_up_count(cursor_col);

    if (limit == 0u) limit = 1u;
    if (cursor_depth > limit) cursor_depth = limit;
    if (cursor_depth == 0u) cursor_depth = 1u;
}

static void clear_selection(void)
{
    UINT8 pile = selected_pile;

    selected_pile = SOL_NO_PILE;
    selected_count = 0u;
    if (pile != SOL_NO_PILE) draw_pile(pile);
}

/* --- actions ------------------------------------------------------------ */

static void start_timer(void)
{
    timer_running = 1u;
}

static void new_deal(void)
{
    solitaire_model_init(&solitaire, next_seed, 1u);
    next_seed = (unsigned int)((next_seed + 0x9e37u) & 65535u);
    selected_pile = SOL_NO_PILE;
    selected_count = 0u;
    autoplay = 0u;
    timer_running = 0u;
    timer_frames = 0u;
    timer_seconds = 0u;
    cursor_row = SOL_ROW_TOP;
    cursor_col = 0u;
    cursor_depth = 1u;
    draw_table();
    draw_status();
    place_pointer();
}

static void after_move(UINT8 action)
{
    start_timer();
    if (action == SOL_ACTION_WON) {
        timer_running = 0u;
        audio_sfx(SFX_WIN);
    } else {
        audio_sfx(SFX_CLICK);
        if (solitaire_model_can_autocomplete(&solitaire)) autoplay = 1u;
    }
    clamp_depth();
    draw_status();
    place_pointer();
}

static void draw_from_stock(void)
{
    clear_selection();
    if (solitaire_model_draw(&solitaire) == SOL_ACTION_NONE) {
        audio_sfx(SFX_ERROR);
        return;
    }
    start_timer();
    audio_sfx(SFX_MOVE);
    draw_stock();
    draw_waste();
    draw_status();
    place_pointer();
}

static void press_a(void)
{
    UINT8 pile;
    UINT8 from;
    UINT8 action;

    if (solitaire_model_is_won(&solitaire) || cursor_row == SOL_ROW_MENU) {
        audio_sfx(SFX_CLICK);
        new_deal();
        return;
    }

    pile = cursor_pile();
    if (pile == SOL_NO_PILE) {
        audio_sfx(SFX_ERROR);
        return;
    }

    if (selected_pile == SOL_NO_PILE) {
        if (pile == SOL_PILE_STOCK) {
            draw_from_stock();
        } else if (solitaire_model_pile_size(&solitaire, pile) != 0u) {
            selected_pile = pile;
            selected_count = (cursor_row == SOL_ROW_TABLEAU) ? cursor_depth : 1u;
            draw_pile(pile);
            audio_sfx(SFX_MOVE);
        } else {
            audio_sfx(SFX_ERROR);
        }
        return;
    }

    if (pile == selected_pile) {
        clear_selection();
        audio_sfx(SFX_MOVE);
        return;
    }

    from = selected_pile;
    action = solitaire_model_move(&solitaire, from, selected_count, pile);
    if (action == SOL_ACTION_NONE) {
        audio_sfx(SFX_ERROR);
        return;
    }
    selected_pile = SOL_NO_PILE;
    selected_count = 0u;
    draw_pile(from);
    draw_pile(pile);
    if (from == SOL_PILE_WASTE) draw_stock();
    cursor_depth = 1u;
    after_move(action);
}

static void press_b(void)
{
    UINT8 pile;
    UINT8 target;

    if (selected_pile != SOL_NO_PILE) {
        clear_selection();
        audio_sfx(SFX_MOVE);
        return;
    }
    pile = cursor_pile();
    if (pile == SOL_NO_PILE || pile == SOL_PILE_STOCK ||
        (pile >= SOL_PILE_FOUNDATION && pile < SOL_PILE_TABLEAU)) {
        audio_sfx(SFX_ERROR);
        return;
    }
    target = solitaire_model_auto_foundation(&solitaire, pile);
    if (target == SOL_NO_PILE) {
        audio_sfx(SFX_ERROR);
        return;
    }
    draw_pile(pile);
    draw_pile(target);
    cursor_depth = 1u;
    after_move(solitaire_model_is_won(&solitaire) ? SOL_ACTION_WON : SOL_ACTION_CHANGED);
}

static void navigate(UINT8 directions)
{
    if (directions & J_LEFT) {
        if (cursor_row == SOL_ROW_MENU) return;
        if (cursor_col != 0u) --cursor_col;
        if (cursor_row == SOL_ROW_TOP && cursor_col == SOL_EMPTY_COLUMN) cursor_col = 1u;
        cursor_depth = 1u;
    } else if (directions & J_RIGHT) {
        if (cursor_row == SOL_ROW_MENU) return;
        if (cursor_col < (UINT8)(SOL_TABLEAU_COUNT - 1u)) ++cursor_col;
        if (cursor_row == SOL_ROW_TOP && cursor_col == SOL_EMPTY_COLUMN) cursor_col = 3u;
        cursor_depth = 1u;
    } else if (directions & J_UP) {
        if (cursor_row == SOL_ROW_TABLEAU) {
            if (selected_pile == SOL_NO_PILE && cursor_depth < face_up_count(cursor_col)) {
                ++cursor_depth;
            } else {
                cursor_row = SOL_ROW_TOP;
                if (cursor_col == SOL_EMPTY_COLUMN) cursor_col = 1u;
            }
        } else if (cursor_row == SOL_ROW_TOP) {
            cursor_row = SOL_ROW_MENU;
        }
    } else if (directions & J_DOWN) {
        if (cursor_row == SOL_ROW_MENU) {
            cursor_row = SOL_ROW_TOP;
        } else if (cursor_row == SOL_ROW_TOP) {
            cursor_row = SOL_ROW_TABLEAU;
            cursor_depth = 1u;
        } else if (cursor_depth > 1u) {
            --cursor_depth;
        }
    } else {
        return;
    }
    audio_sfx(SFX_MOVE);
    place_pointer();
}

static void tick_autoplay(void)
{
    UINT8 source;

    if (!autoplay) return;
    if (++autoplay_wait < 6u) return;
    autoplay_wait = 0u;
    source = solitaire_model_autocomplete_step(&solitaire);
    if (source == SOL_NO_PILE) {
        autoplay = 0u;
        return;
    }
    draw_pile(source);
    for (source = 0u; source != SOL_FOUNDATION_COUNT; ++source) draw_foundation(source);
    if (solitaire_model_is_won(&solitaire)) {
        autoplay = 0u;
        after_move(SOL_ACTION_WON);
    } else {
        audio_sfx(SFX_CLICK);
        draw_status();
    }
}

static void tick_timer(void)
{
    if (!timer_running || timer_seconds >= 9999u) return;
    if (++timer_frames < 60u) return;
    timer_frames = 0u;
    ++timer_seconds;
    draw_time();
}

void solitaire_enter(void) BANKED
{
    ui_scene_begin();
    set_bkg_palette(PAL_APP_A, 1u, &solitaire_palettes[0]);
    set_bkg_palette(PAL_APP_B, 1u, &solitaire_palettes[4]);
    set_bkg_palette(PAL_MONO, 1u, &solitaire_palettes[8]);
    set_bkg_palette(PAL_APP_C, 1u, &solitaire_palettes[12]);
    build_card_art();

    ui_window(0u, 0u, SCREEN_TILES_W, SCREEN_TILES_H, "Solitaire", 1u, UI_CLIENT_FACE);
    ui_menu(1u, 1u, 18u, "Game  Help");
    ui_fill(1u, SOL_TOP_Y, 18u, (UINT8)(SOL_STATUS_Y - SOL_TOP_Y), TILE_BLANK, PAL_CARD_BLACK);
    ui_label_init(&score_label, 1u, SOL_STATUS_Y, 9u, PAL_WINDOW);
    ui_label_init(&time_label, 10u, SOL_STATUS_Y, 9u, PAL_WINDOW);

    pointer_reset(0u, 0u);
    new_deal();
    pointer_show();
    ui_scene_end();
}

AppState solitaire_update(const InputState *input) BANKED
{
    if (input->pressed & J_START) {
        pointer_hide();
        return APP_DESKTOP;
    }

    tick_timer();
    if (autoplay) {
        tick_autoplay();
        return APP_SOLITAIRE;
    }

    navigate(input->repeated);
    if (input->pressed & J_A) press_a();
    else if (input->pressed & J_B) press_b();
    else if (input->pressed & J_SELECT) draw_from_stock();
    return APP_SOLITAIRE;
}
