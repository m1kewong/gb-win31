#include <gb/gb.h>

#include "assets.h"
#include "audio.h"
#include "boot.h"
#include "ui.h"

static UINT8 dos_step;
static UINT8 splash_frames;

static void boot_line(UINT8 y, const char *text)
{
    ui_text(1u, y, text, PAL_MONO);
}

void boot_enter(void)
{
    ui_scene_begin();
    pointer_hide();
    ui_clear(PAL_MONO);
    boot_line(0u, "GB WORKBENCH BIOS");
    boot_line(1u, "VERSION 4.5.0");
    boot_line(3u, "SYSTEM CONFIG");
    boot_line(5u, "CPU  SHARP LR35902");
    boot_line(6u, "CLOCK       8.38MHZ");
    boot_line(7u, "WORK RAM       32KB");
    boot_line(8u, "VIDEO  CGB 160X144");
    boot_line(10u, "CARTRIDGE ROM  OK");
    boot_line(11u, "MEMORY TEST    OK");
    boot_line(14u, "A: CONTINUE");
    boot_line(15u, "START: QUICK BOOT");
    ui_scene_end();
    audio_sfx(SFX_BOOT);
}

AppState boot_update(const InputState *input)
{
    if (input->pressed & J_START) return APP_DESKTOP;
    if (input->pressed & J_A) return APP_DOS;
    return APP_BOOT;
}

static void dos_draw_step(void)
{
    switch (dos_step) {
        case 0u: ui_text(0u, 0u, "GB-DOS VERSION 3.10", PAL_MONO); break;
        case 1u: ui_text(0u, 2u, "ROM DEVICE DRIVER OK", PAL_MONO); break;
        case 2u: ui_text(0u, 3u, "LOADING GBSDRV.EXE", PAL_MONO); break;
        case 3u: ui_text(0u, 4u, "MOUSE DRIVER FOUND", PAL_MONO); break;
        case 4u: ui_text(0u, 7u, "C:\\>CD WORKBENCH", PAL_MONO); break;
        case 5u: ui_text(0u, 9u, "C:\\WORKBENCH>WIN", PAL_MONO); break;
        default:
            ui_scene_begin();
            ui_clear(PAL_DESKTOP);
            ui_window(2u, 3u, 16u, 11u, "GBWORKBENCH", 1u);
            ui_text(5u, 7u, "GB WORKBENCH", PAL_TITLE_ACTIVE);
            ui_text(4u, 9u, "STARTING...", PAL_WINDOW);
            ui_scene_end();
            splash_frames = 0u;
            break;
    }
}

void dos_enter(void)
{
    ui_scene_begin();
    pointer_hide();
    ui_clear(PAL_MONO);
    dos_step = 0u;
    splash_frames = 0u;
    dos_draw_step();
    ui_text(0u, 16u, "A: TYPE  START: SKIP", PAL_MONO);
    ui_scene_end();
}

AppState dos_update(const InputState *input)
{
    if (input->pressed & J_START) return APP_DESKTOP;

    if (dos_step > 5u) {
        ++splash_frames;
        if (splash_frames >= 75u || (input->pressed & J_A)) return APP_DESKTOP;
        return APP_DOS;
    }

    if (input->pressed & J_A) {
        ++dos_step;
        dos_draw_step();
        audio_sfx(SFX_CLICK);
    }
    return APP_DOS;
}
