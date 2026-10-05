#pragma bank 255

#include <gb/gb.h>

#include "assets.h"
#include "audio.h"
#include "desktop.h"
#include "piano.h"
#include "ui.h"

#define PIANO_KEY_COUNT 8u
#define PIANO_TONE_COUNT 3u

#define PIANO_WIN_X 1u
#define PIANO_WIN_Y 3u
#define PIANO_WIN_W 18u
#define PIANO_WIN_H 12u
#define PIANO_KEYS_X 2u
#define PIANO_KEYS_Y 6u
#define PIANO_UPPER_ROWS 3u
#define PIANO_LOWER_ROWS 3u

#define PIANO_REGION_UPPER 0u
#define PIANO_REGION_LOWER 1u
#define PIANO_REGION_BOTTOM 2u
#define PIANO_EDGE_NONE 0u
#define PIANO_EDGE_BLACK 1u
#define PIANO_EDGE_LAST 2u

static const UINT8 piano_notes[PIANO_KEY_COUNT] = {
    0u, 2u, 4u, 5u, 7u, 9u, 11u, 12u
};

static const char * const piano_note_names[PIANO_KEY_COUNT] = {
    "Key: C", "Key: D", "Key: E", "Key: F", "Key: G", "Key: A", "Key: B", "Key: C'"
};

static const char * const piano_tone_names[PIANO_TONE_COUNT] = {
    "Tone: Thin", "Tone: Mellow", "Tone: Full"
};

static const UINT8 piano_tone_duty[PIANO_TONE_COUNT] = {
    0x00u, 0x40u, 0x80u
};

/* Exported so the emulator smoke test can observe the selected key. */
UINT8 piano_key;
static UINT8 piano_tone;
static UiLabel tone_label;
static UiLabel key_label;

static UINT8 key_tile(UINT8 region, UINT8 half, UINT8 edge, UINT8 pressed)
{
    return (UINT8)(region * 12u + half * 6u + edge * 2u + pressed);
}

static void build_key_tile(UINT8 region, UINT8 half, UINT8 edge, UINT8 pressed)
{
    UINT8 row;
    UINT8 column;
    char pixel;

    for (row = 0u; row != 8u; ++row) {
        for (column = 0u; column != 8u; ++column) {
            pixel = pressed ? '1' : '0';
            if (half == 0u && column == 0u) pixel = '3';
            if (half == 1u && edge == PIANO_EDGE_LAST && column == 7u) pixel = '3';
            if (region == PIANO_REGION_UPPER && edge == PIANO_EDGE_BLACK &&
                ((half == 0u && column <= 2u) || (half == 1u && column >= 5u))) {
                pixel = '3';
            }
            if (region == PIANO_REGION_BOTTOM && row == 7u) pixel = '3';
            ui_art_scratch[(UINT8)(row * 8u + column)] = pixel;
        }
    }
    ui_art_load(1u, key_tile(region, half, edge, pressed), 1u, ui_art_scratch);
}

static void build_key_art(void)
{
    UINT8 region;
    UINT8 edge;
    UINT8 pressed;

    for (region = 0u; region != 3u; ++region) {
        for (pressed = 0u; pressed != 2u; ++pressed) {
            for (edge = 0u; edge != 2u; ++edge) build_key_tile(region, 0u, edge, pressed);
            for (edge = 0u; edge != 3u; ++edge) build_key_tile(region, 1u, edge, pressed);
        }
    }
}

static UINT8 piano_has_black_key(UINT8 key)
{
    return (UINT8)(key == 0u || key == 1u || key == 3u ||
                   key == 4u || key == 5u);
}

static void draw_key(UINT8 key)
{
    UINT8 row;
    UINT8 region;
    UINT8 pressed = (UINT8)(key == piano_key);
    UINT8 left_edge = (key != 0u && piano_has_black_key((UINT8)(key - 1u))) ?
        PIANO_EDGE_BLACK : PIANO_EDGE_NONE;
    UINT8 right_edge;
    UINT8 x = (UINT8)(PIANO_KEYS_X + (UINT8)(key << 1u));

    if (key == (UINT8)(PIANO_KEY_COUNT - 1u)) right_edge = PIANO_EDGE_LAST;
    else if (piano_has_black_key(key)) right_edge = PIANO_EDGE_BLACK;
    else right_edge = PIANO_EDGE_NONE;

    for (row = 0u; row != (UINT8)(PIANO_UPPER_ROWS + PIANO_LOWER_ROWS); ++row) {
        if (row < PIANO_UPPER_ROWS) region = PIANO_REGION_UPPER;
        else if (row == (UINT8)(PIANO_UPPER_ROWS + PIANO_LOWER_ROWS - 1u)) region = PIANO_REGION_BOTTOM;
        else region = PIANO_REGION_LOWER;

        ui_set_art(x, (UINT8)(PIANO_KEYS_Y + row),
                   key_tile(region, 0u,
                            (region == PIANO_REGION_UPPER) ? left_edge : PIANO_EDGE_NONE,
                            pressed),
                   PAL_WINDOW);
        ui_set_art((UINT8)(x + 1u), (UINT8)(PIANO_KEYS_Y + row),
                   key_tile(region, 1u,
                            (region == PIANO_REGION_UPPER || right_edge == PIANO_EDGE_LAST) ?
                                right_edge : PIANO_EDGE_NONE,
                            pressed),
                   PAL_WINDOW);
    }
}

static void draw_status(void)
{
    ui_label_set(&tone_label, piano_tone_names[piano_tone], TEXT_INK_ON_PAPER,
                 TEXT_RULE_BOTTOM);
    ui_label_set(&key_label, piano_note_names[piano_key], TEXT_INK_ON_PAPER,
                 (UINT8)(TEXT_ALIGN_RIGHT | TEXT_RULE_BOTTOM));
}

static void piano_play_key(void)
{
    audio_note(piano_notes[piano_key], 32u, piano_tone_duty[piano_tone]);
}

void piano_enter(void) BANKED
{
    UINT8 key;

    ui_scene_begin();
    pointer_hide();
    build_key_art();
    desktop_draw_backdrop();
    ui_window(PIANO_WIN_X, PIANO_WIN_Y, PIANO_WIN_W, PIANO_WIN_H, "Piano", 1u,
              UI_CLIENT_WHITE);
    ui_menu(2u, 4u, 16u, "File  Tone  Help");
    ui_label_init(&tone_label, 2u, 5u, 10u, PAL_WINDOW);
    ui_label_init(&key_label, 12u, 5u, 6u, PAL_WINDOW);
    ui_status(2u, 13u, 16u, "A: play   Select: tone");

    piano_key = 0u;
    piano_tone = 1u;
    draw_status();
    for (key = 0u; key != PIANO_KEY_COUNT; ++key) draw_key(key);
    ui_scene_end();
}

AppState piano_update(const InputState *input) BANKED
{
    UINT8 old_key;
    UINT8 directions;

    if (input->pressed & J_START) return APP_DESKTOP;

    old_key = piano_key;
    directions = input->repeated;

    if (directions & J_LEFT) {
        piano_key = (piano_key == 0u) ?
            (UINT8)(PIANO_KEY_COUNT - 1u) : (UINT8)(piano_key - 1u);
    } else if (directions & J_RIGHT) {
        piano_key = (UINT8)((piano_key + 1u) % PIANO_KEY_COUNT);
    } else if (directions & J_UP) {
        piano_key = (piano_key < 4u) ? (UINT8)(piano_key + 4u) : piano_key;
    } else if (directions & J_DOWN) {
        piano_key = (piano_key >= 4u) ? (UINT8)(piano_key - 4u) : piano_key;
    }

    if (old_key != piano_key) {
        audio_sfx(SFX_MOVE);
        draw_key(old_key);
        draw_key(piano_key);
        draw_status();
    }

    if (input->pressed & J_SELECT) {
        piano_tone = (UINT8)((piano_tone + 1u) % PIANO_TONE_COUNT);
        draw_status();
        piano_play_key();
    }

    if (input->pressed & J_A) piano_play_key();

    return APP_PIANO;
}
