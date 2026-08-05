#include <gb/gb.h>

#include "assets.h"
#include "audio.h"
#include "piano.h"
#include "ui.h"

#define PIANO_KEY_COUNT 8u
#define PIANO_TONE_COUNT 3u
#define PIANO_KEY_X 6u
#define PIANO_KEY_TOP 9u
#define PIANO_KEY_HEIGHT 5u

static const UINT8 piano_notes[PIANO_KEY_COUNT] = {
    0u, 2u, 4u, 5u, 7u, 9u, 11u, 12u
};

static const char * const piano_note_names[PIANO_KEY_COUNT] = {
    "C", "D", "E", "F", "G", "A", "B", "C+"
};

static const char * const piano_tone_names[PIANO_TONE_COUNT] = {
    "THIN", "MELLOW", "FULL"
};

static const UINT8 piano_tone_duty[PIANO_TONE_COUNT] = {
    0x00u, 0x40u, 0x80u
};

static UINT8 piano_key;
static UINT8 piano_tone;

static UINT8 piano_has_black_key(UINT8 key)
{
    return (UINT8)(key == 0u || key == 1u || key == 3u ||
                   key == 4u || key == 5u);
}

static void piano_draw_status(void)
{
    ui_text_clipped(3u, 4u, "SELECT:TONE", PAL_WINDOW, 11u);
    ui_text_clipped(3u, 5u, piano_tone_names[piano_tone], PAL_TITLE_ACTIVE, 7u);
    ui_text_clipped(12u, 5u, piano_note_names[piano_key], PAL_WINDOW, 3u);
    ui_text_clipped(3u, 15u, "A:PLAY ST:DESK", PAL_WINDOW, 14u);
}

static void piano_draw_keyboard(void)
{
    UINT8 key;
    UINT8 row;
    UINT8 tile;

    ui_fill(PIANO_KEY_X, 8u, PIANO_KEY_COUNT, 1u, TILE_BLANK, PAL_WINDOW);

    for (key = 0u; key != PIANO_KEY_COUNT; ++key) {
        tile = (key == piano_key) ? TILE_KEY_WHITE_ACTIVE : TILE_KEY_WHITE;
        for (row = 0u; row != PIANO_KEY_HEIGHT; ++row) {
            ui_set_tile((UINT8)(PIANO_KEY_X + key),
                        (UINT8)(PIANO_KEY_TOP + row), tile, PAL_WINDOW);
        }
        if (key < (UINT8)(PIANO_KEY_COUNT - 1u) && piano_has_black_key(key)) {
            ui_set_tile((UINT8)(PIANO_KEY_X + key + 1u), 8u,
                        TILE_KEY_BLACK, PAL_WINDOW);
        }
    }
}

static void piano_play_key(void)
{
    audio_note(piano_notes[piano_key], 32u, piano_tone_duty[piano_tone]);
}

void piano_enter(void)
{
    ui_scene_begin();
    pointer_hide();
    ui_clear(PAL_DESKTOP);
    ui_window(1u, 1u, 18u, 16u, "PIANO", 1u);
    ui_menu(2u, 2u, 16u, "FILE  TONE  HELP");
    ui_text_clipped(3u, 3u, "WORKBENCH KEYS", PAL_WINDOW, 14u);

    piano_key = 0u;
    piano_tone = 1u;
    piano_draw_status();
    piano_draw_keyboard();
    ui_scene_end();
}

AppState piano_update(const InputState *input)
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
        piano_key = (piano_key < 4u) ?
            (UINT8)(piano_key + 4u) : piano_key;
    } else if (directions & J_DOWN) {
        piano_key = (piano_key >= 4u) ?
            (UINT8)(piano_key - 4u) : piano_key;
    }

    if (old_key != piano_key) {
        audio_sfx(SFX_MOVE);
        piano_draw_keyboard();
        piano_draw_status();
    }

    if (input->pressed & J_SELECT) {
        piano_tone = (UINT8)((piano_tone + 1u) % PIANO_TONE_COUNT);
        piano_draw_status();
        piano_play_key();
    }

    if (input->pressed & J_A) piano_play_key();

    return APP_PIANO;
}
