#include <gb/gb.h>
#include <gb/cgb.h>

#include "app.h"
#include "audio.h"
#include "boot.h"
#include "desktop.h"
#include "input.h"
#include "minesweeper.h"
#include "paint.h"
#include "piano.h"
#include "media.h"
#include "cannon.h"
#include "ui.h"

static void enter_state(AppState state)
{
    switch (state) {
        case APP_BOOT: boot_enter(); break;
        case APP_DOS: dos_enter(); break;
        case APP_DESKTOP: desktop_enter(); break;
        case APP_SWEEPER: minesweeper_enter(); break;
        case APP_PAINT: paint_enter(); break;
        case APP_PIANO: piano_enter(); break;
        case APP_MEDIA: media_enter(); break;
        case APP_CANNON: cannon_enter(); break;
        default: desktop_enter(); break;
    }
}

static AppState update_state(AppState state, const InputState *input)
{
    switch (state) {
        case APP_BOOT: return boot_update(input);
        case APP_DOS: return dos_update(input);
        case APP_DESKTOP: return desktop_update(input);
        case APP_SWEEPER: return minesweeper_update(input);
        case APP_PAINT: return paint_update(input);
        case APP_PIANO: return piano_update(input);
        case APP_MEDIA: return media_update(input);
        case APP_CANNON: return cannon_update(input);
        default: return APP_DESKTOP;
    }
}

void main(void)
{
    AppState current_state;
    AppState next_state;

    cpu_fast();
    audio_init();
    input_init();
    ui_init();

    current_state = APP_BOOT;
    enter_state(current_state);

    while (1) {
        vsync();
        input_update();
        next_state = update_state(current_state, input_get());
        audio_tick();
        if (next_state != current_state) {
            current_state = next_state;
            enter_state(current_state);
        }
    }
}
