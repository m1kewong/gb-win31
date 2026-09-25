#pragma bank 255

#include <gb/gb.h>
#include <gb/cgb.h>

#include "assets.h"
#include "audio.h"
#include "desktop.h"
#include "media.h"
#include "ui.h"

#define MEDIA_TRACK_COUNT 4u
#define MEDIA_CONTROL_COUNT 4u
#define MEDIA_PREVIOUS 0u
#define MEDIA_PLAY 1u
#define MEDIA_STOP 2u
#define MEDIA_NEXT 3u

#define MEDIA_WIN_X 2u
#define MEDIA_WIN_Y 5u
#define MEDIA_WIN_W 16u
#define MEDIA_WIN_H 9u
#define MEDIA_LCD_X 4u
#define MEDIA_LCD_Y 8u
#define MEDIA_LCD_W 12u
#define MEDIA_SLIDER_Y 11u
#define MEDIA_BUTTONS_Y 12u

#define MEDIA_ART_BUTTONS 0u
#define MEDIA_ART_TRACK 16u
#define MEDIA_ART_THUMB 17u

#define PAL_MEDIA_LCD PAL_APP_A

static const palette_color_t media_lcd_palette[] = {
    RGB(0, 0, 0), RGB(6, 31, 6), RGB(0, 10, 0), RGB(31, 31, 31)
};

/* Clean-room track names; none refer to commercial music. */
static const char * const media_track_names[MEDIA_TRACK_COUNT] = {
    "Cyan Horizon",
    "Modem Moon",
    "Cursor March",
    "Night Signal"
};

/* Transport glyphs on a 16x8 button face: previous, play, stop, next. */
static const char media_glyphs[] =
    "................" "...#...#...#...." "...#..##..##...." "...#.###.###...."
    "...#.###.###...." "...#..##..##...." "...#...#...#...." "................"
    "................" "......#........." "......##........" "......###......."
    "......###......." "......##........" "......#........." "................"
    "................" "................" ".....######....." ".....######....."
    ".....######....." ".....######....." "................" "................"
    "................" "....#...#...#..." "....##..##..#..." "....###.###.#..."
    "....###.###.#..." "....##..##..#..." "....#...#...#..." "................";

static const char media_slider_art[] =
    "11111111" "11111111" "11111111" "22222222"
    "00000000" "11111111" "11111111" "11111111"
    "10000031" "10111131" "10111131" "10111131"
    "10111131" "10111131" "10111131" "13333331";

/* Exported so the emulator smoke test can observe playback state. */
UINT8 media_track;
UINT8 media_playing;
static UINT8 media_control;
static UINT8 media_progress;
static UINT8 media_progress_wait;
static UiLabel status_label;
static UiLabel track_label;
static char media_art[64];

static void build_button_art(void)
{
    UINT8 glyph;
    UINT8 pressed;
    UINT8 half;
    UINT8 row;
    UINT8 column;
    UINT8 x;
    char pixel;
    char light;
    char dark;

    for (glyph = 0u; glyph != MEDIA_CONTROL_COUNT; ++glyph) {
        for (pressed = 0u; pressed != 2u; ++pressed) {
            light = pressed ? '3' : '0';
            dark = pressed ? '0' : '3';
            for (half = 0u; half != 2u; ++half) {
                for (row = 0u; row != 8u; ++row) {
                    for (column = 0u; column != 8u; ++column) {
                        x = (UINT8)((half << 3u) + column);
                        pixel = '1';
                        if (media_glyphs[(UINT16)glyph * 128u + row * 16u + x] == '#') pixel = '3';
                        if (row == 0u || x == 0u) pixel = light;
                        if (row == 7u || x == 15u) pixel = dark;
                        media_art[(UINT8)(row * 8u + column)] = pixel;
                    }
                }
                ui_art_load(1u, (UINT8)(MEDIA_ART_BUTTONS + glyph * 4u + pressed * 2u + half),
                            1u, media_art);
            }
        }
    }
    ui_art_load(1u, MEDIA_ART_TRACK, 2u, media_slider_art);
}

static void media_draw_progress(void)
{
    UINT8 x;
    UINT8 position = (UINT8)(media_progress % MEDIA_LCD_W);

    for (x = 0u; x != MEDIA_LCD_W; ++x) {
        ui_set_art((UINT8)(MEDIA_LCD_X + x), MEDIA_SLIDER_Y,
                   (x == position) ? MEDIA_ART_THUMB : MEDIA_ART_TRACK, PAL_WINDOW);
    }
}

static void media_draw_display(void)
{
    UINT8 lcd = TEXT_COLORS(0u, 1u, 0u);

    ui_label_set(&status_label, media_playing ? "Playing" : "Stopped", lcd, 0u);
    ui_label_set(&track_label, media_track_names[media_track], lcd, 0u);
    media_draw_progress();
}

static void media_draw_controls(void)
{
    UINT8 control;
    UINT8 first;
    UINT8 x;

    for (control = 0u; control != MEDIA_CONTROL_COUNT; ++control) {
        first = (UINT8)(MEDIA_ART_BUTTONS + control * 4u +
                        ((control == media_control) ? 2u : 0u));
        x = (UINT8)(MEDIA_LCD_X + 1u + control * 3u);
        ui_set_art(x, MEDIA_BUTTONS_Y, first, PAL_WINDOW);
        ui_set_art((UINT8)(x + 1u), MEDIA_BUTTONS_Y, (UINT8)(first + 1u), PAL_WINDOW);
    }
}

static void media_change_track(INT8 direction)
{
    if (direction < 0) {
        media_track = (media_track == 0u) ?
            (UINT8)(MEDIA_TRACK_COUNT - 1u) : (UINT8)(media_track - 1u);
    } else {
        media_track = (UINT8)((media_track + 1u) % MEDIA_TRACK_COUNT);
    }

    media_progress = 0u;
    media_progress_wait = 0u;
    if (media_playing) audio_music_set((UINT8)(media_track + 1u));
    audio_sfx(SFX_MOVE);
    media_draw_display();
}

static void media_stop(void)
{
    media_playing = 0u;
    audio_music_set(0u);
    media_draw_display();
}

static void media_activate_control(void)
{
    if (media_control == MEDIA_PREVIOUS) {
        media_change_track(-1);
    } else if (media_control == MEDIA_PLAY) {
        audio_sfx(SFX_CLICK);
        media_playing = 1u;
        media_progress = 0u;
        media_progress_wait = 0u;
        audio_music_set((UINT8)(media_track + 1u));
        media_draw_display();
    } else if (media_control == MEDIA_STOP) {
        audio_sfx(SFX_CLICK);
        media_stop();
    } else {
        media_change_track(1);
    }
}

void media_enter(void) BANKED
{
    UINT8 active_track;

    ui_scene_begin();
    pointer_hide();
    set_bkg_palette(PAL_MEDIA_LCD, 1u, media_lcd_palette);
    build_button_art();
    desktop_draw_backdrop();
    ui_window(MEDIA_WIN_X, MEDIA_WIN_Y, MEDIA_WIN_W, MEDIA_WIN_H, "Media Player", 1u,
              UI_CLIENT_FACE);
    ui_menu(3u, 6u, 14u, "File  View  Help");
    ui_sunken(MEDIA_LCD_X, MEDIA_LCD_Y, MEDIA_LCD_W, 2u);
    ui_label_init(&status_label, MEDIA_LCD_X, MEDIA_LCD_Y, MEDIA_LCD_W, PAL_MEDIA_LCD);
    ui_label_init(&track_label, MEDIA_LCD_X, (UINT8)(MEDIA_LCD_Y + 1u), MEDIA_LCD_W,
                  PAL_MEDIA_LCD);

    active_track = audio_music_get();
    if (active_track != 0u) {
        media_track = (UINT8)(active_track - 1u);
        media_playing = 1u;
    } else {
        media_playing = 0u;
    }

    media_control = media_playing ? MEDIA_STOP : MEDIA_PLAY;
    media_progress = 0u;
    media_progress_wait = 0u;
    media_draw_display();
    media_draw_controls();
    ui_scene_end();
}

AppState media_update(const InputState *input) BANKED
{
    UINT8 old_control;

    /* Deliberately leave audio_music_set untouched: music follows desktop. */
    if (input->pressed & J_START) return APP_DESKTOP;

    old_control = media_control;
    if (input->repeated & J_LEFT) {
        media_control = (media_control == 0u) ?
            (UINT8)(MEDIA_CONTROL_COUNT - 1u) : (UINT8)(media_control - 1u);
    } else if (input->repeated & J_RIGHT) {
        media_control = (UINT8)((media_control + 1u) % MEDIA_CONTROL_COUNT);
    }

    if (input->pressed & J_SELECT) {
        media_control = (UINT8)((media_control + 1u) % MEDIA_CONTROL_COUNT);
    }

    if (old_control != media_control) {
        audio_sfx(SFX_MOVE);
        media_draw_controls();
    }

    if (input->repeated & J_UP) media_change_track(-1);
    else if (input->repeated & J_DOWN) media_change_track(1);

    if (input->pressed & J_A) media_activate_control();
    if (input->pressed & J_B) {
        media_control = MEDIA_STOP;
        media_stop();
        media_draw_controls();
    }

    if (media_playing) {
        ++media_progress_wait;
        if (media_progress_wait >= 8u) {
            media_progress_wait = 0u;
            media_progress = (UINT8)((media_progress + 1u) % MEDIA_LCD_W);
            media_draw_progress();
        }
    }

    return APP_MEDIA;
}
