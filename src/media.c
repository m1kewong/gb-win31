#include <gb/gb.h>

#include "assets.h"
#include "audio.h"
#include "media.h"
#include "ui.h"

#define MEDIA_TRACK_COUNT 4u
#define MEDIA_CONTROL_COUNT 4u
#define MEDIA_PREVIOUS 0u
#define MEDIA_PLAY 1u
#define MEDIA_STOP 2u
#define MEDIA_NEXT 3u
#define MEDIA_PROGRESS_WIDTH 12u

/* Clean-room track names; none refer to commercial music. */
static const char * const media_track_names[MEDIA_TRACK_COUNT] = {
    "CYAN HORIZON",
    "MODEM MOON",
    "CURSOR MARCH",
    "NIGHT SIGNAL"
};

static const char * const media_control_names[MEDIA_CONTROL_COUNT] = {
    "<<", "PLAY", "STOP", ">>"
};

static const UINT8 media_control_x[MEDIA_CONTROL_COUNT] = {
    3u, 6u, 11u, 16u
};

static const UINT8 media_control_width[MEDIA_CONTROL_COUNT] = {
    2u, 4u, 4u, 2u
};

static UINT8 media_track;
static UINT8 media_control;
static UINT8 media_playing;
static UINT8 media_progress;
static UINT8 media_progress_wait;

static void media_draw_progress(void)
{
    UINT8 position;
    UINT8 x;

    for (x = 0u; x != MEDIA_PROGRESS_WIDTH; ++x) {
        ui_set_tile((UINT8)(4u + x), 8u, TILE_BLANK, PAL_DESKTOP);
    }

    if (media_playing) {
        position = (UINT8)(media_progress % MEDIA_PROGRESS_WIDTH);
        ui_set_tile((UINT8)(4u + position), 8u, TILE_SOLID, PAL_DESKTOP);
    }
}

static void media_draw_display(void)
{
    ui_fill(3u, 4u, 14u, 5u, TILE_BLANK, PAL_DESKTOP);
    ui_text_clipped(4u, 5u, media_playing ? "NOW PLAYING" : "PLAYER READY",
                    PAL_DESKTOP, 12u);
    ui_text_clipped(4u, 6u, media_track_names[media_track],
                    PAL_DESKTOP, 12u);
    ui_text_clipped(4u, 7u, media_playing ? "STEREO  ON" : "STEREO OFF",
                    PAL_DESKTOP, 12u);
    media_draw_progress();
}

static void media_draw_controls(void)
{
    UINT8 control;

    for (control = 0u; control != MEDIA_CONTROL_COUNT; ++control) {
        ui_text_clipped(media_control_x[control], 11u,
                        media_control_names[control],
                        (control == media_control) ? PAL_TITLE_ACTIVE : PAL_WINDOW,
                        media_control_width[control]);
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
        media_playing = 0u;
        audio_music_set(0u);
        media_draw_display();
    } else {
        media_change_track(1);
    }
}

void media_enter(void)
{
    UINT8 active_track;

    ui_scene_begin();
    pointer_hide();
    ui_clear(PAL_DESKTOP);
    ui_window(1u, 1u, 18u, 16u, "MEDIA PLAYER", 1u);
    ui_menu(2u, 2u, 16u, "FILE  VIEW  HELP");

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
    ui_text_clipped(3u, 13u, "UP/DN:TRACK", PAL_WINDOW, 13u);
    ui_text_clipped(3u, 15u, "START:DESKTOP", PAL_WINDOW, 13u);
    ui_scene_end();
}

AppState media_update(const InputState *input)
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
        media_playing = 0u;
        media_control = MEDIA_STOP;
        audio_music_set(0u);
        media_draw_display();
        media_draw_controls();
    }

    if (media_playing) {
        ++media_progress_wait;
        if (media_progress_wait >= 8u) {
            media_progress_wait = 0u;
            media_progress = (UINT8)((media_progress + 1u) % MEDIA_PROGRESS_WIDTH);
            media_draw_progress();
        }
    }

    return APP_MEDIA;
}
