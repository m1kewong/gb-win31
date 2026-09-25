#pragma bank 255

#include <gb/gb.h>
#include <gb/cgb.h>

#include "assets.h"
#include "audio.h"
#include "cannon.h"
#include "text.h"
#include "ui.h"

#define CANNON_FIELD_X 2u
#define CANNON_FIELD_Y 5u
#define CANNON_FIELD_W 16u
#define CANNON_FIELD_H 10u
#define CANNON_ROW 14u
#define CANNON_TARGET_BOTTOM 13u
#define CANNON_SCORE_X 2u
#define CANNON_HEADER_Y 2u
#define CANNON_ART_LED 0u
#define CANNON_TARGET_COUNT 3u
#define CANNON_TARGET_PERIOD 24u
#define CANNON_SHOT_PERIOD 2u
#define CANNON_WIN_SCORE 10u

#define CANNON_PLAYING 0u
#define CANNON_WON 1u
#define CANNON_LOST 2u

typedef struct CannonTarget {
    UINT8 x;
    UINT8 y;
} CannonTarget;

static const UINT8 cannon_lane_x[CANNON_TARGET_COUNT] = {
    4u, 9u, 14u
};

static const palette_color_t cannon_led_palette[] = {
    RGB(24, 24, 24), RGB(31, 31, 31), RGB(31, 0, 0), RGB(0, 0, 0)
};

static CannonTarget cannon_targets[CANNON_TARGET_COUNT];
static UINT8 cannon_x;
/* Exported so the emulator smoke test can observe the score. */
UINT8 cannon_score;
UINT8 cannon_lives;
static UiLabel lives_label;
static UiLabel message_label;
static UiLabel restart_label;
static UINT8 cannon_state;
static UINT8 cannon_target_wait;
static UINT8 cannon_spawn_phase;
static UINT8 cannon_shot_active;
static UINT8 cannon_shot_x;
static UINT8 cannon_shot_y;
static UINT8 cannon_shot_wait;

static UINT8 cannon_hit_target(void);

static void cannon_blank(UINT8 x, UINT8 y)
{
    ui_set_tile(x, y, TILE_BLANK, PAL_MONO);
}

static void cannon_draw_status(void)
{
    static char lives_text[] = "Lives: 0";

    ui_led_draw(CANNON_SCORE_X, CANNON_HEADER_Y, CANNON_ART_LED, cannon_score, 3u, PAL_APP_C);
    lives_text[7] = (char)('0' + cannon_lives);
    ui_label_set(&lives_label, lives_text, TEXT_COLORS(COLOR_GREY, COLOR_BLACK, 0u),
                 TEXT_ALIGN_RIGHT);
}

static void cannon_draw_target(UINT8 index)
{
    ui_set_tile(cannon_targets[index].x, cannon_targets[index].y,
                TILE_TARGET, PAL_MONO);
}

static void cannon_spawn_target(UINT8 index, UINT8 y)
{
    cannon_targets[index].x = (UINT8)(cannon_lane_x[index] +
                                      ((cannon_spawn_phase + index) & 1u));
    cannon_targets[index].y = y;
    ++cannon_spawn_phase;
}

static void cannon_finish(UINT8 result)
{
    cannon_state = result;
    cannon_shot_active = 0u;
    pointer_hide();
    ui_fill(CANNON_FIELD_X, CANNON_FIELD_Y,
            CANNON_FIELD_W, CANNON_FIELD_H, TILE_BLANK, PAL_MONO);

    ui_label_palette(&message_label, PAL_MONO);
    ui_label_palette(&restart_label, PAL_MONO);
    ui_label_set(&message_label, (result == CANNON_WON) ? "City safe!" : "Defense down",
                 TEXT_COLORS(0u, 3u, 0u), TEXT_ALIGN_CENTER);
    ui_label_set(&restart_label, "A: play again", TEXT_COLORS(0u, 2u, 0u),
                 TEXT_ALIGN_CENTER);
    audio_sfx((result == CANNON_WON) ? SFX_WIN : SFX_ERROR);
}

static void cannon_reset_game(void)
{
    UINT8 index;

    ui_fill(CANNON_FIELD_X, CANNON_FIELD_Y,
            CANNON_FIELD_W, CANNON_FIELD_H, TILE_BLANK, PAL_MONO);
    cannon_score = 0u;
    cannon_lives = 3u;
    cannon_state = CANNON_PLAYING;
    cannon_target_wait = 0u;
    cannon_spawn_phase = 0u;
    cannon_shot_active = 0u;
    cannon_shot_wait = 0u;
    cannon_x = 10u;

    cannon_spawn_target(0u, 6u);
    cannon_spawn_target(1u, 8u);
    cannon_spawn_target(2u, 10u);
    for (index = 0u; index != CANNON_TARGET_COUNT; ++index) {
        cannon_draw_target(index);
    }

    ui_set_tile(cannon_x, CANNON_ROW, TILE_CANNON, PAL_MONO);
    cannon_draw_status();
    pointer_reset((UINT8)(cannon_x * 8u), 72u);
    pointer_show();
}

static void cannon_lose_life(void)
{
    if (cannon_lives != 0u) --cannon_lives;
    cannon_draw_status();
    if (cannon_lives == 0u) cannon_finish(CANNON_LOST);
    else audio_sfx(SFX_ERROR);
}

static void cannon_move_targets(void)
{
    UINT8 index;

    for (index = 0u; index != CANNON_TARGET_COUNT; ++index) {
        cannon_blank(cannon_targets[index].x, cannon_targets[index].y);
        if (cannon_targets[index].y >= CANNON_TARGET_BOTTOM) {
            cannon_spawn_target(index, CANNON_FIELD_Y);
            cannon_lose_life();
            if (cannon_state != CANNON_PLAYING) return;
        } else {
            ++cannon_targets[index].y;
        }
    }

    for (index = 0u; index != CANNON_TARGET_COUNT; ++index) {
        cannon_draw_target(index);
    }
    if (cannon_shot_active && cannon_hit_target()) {
        cannon_shot_active = 0u;
    } else if (cannon_shot_active) {
        ui_set_tile(cannon_shot_x, cannon_shot_y, TILE_CURSOR_MARK, PAL_MONO);
    }
}

static UINT8 cannon_hit_target(void)
{
    UINT8 index;

    for (index = 0u; index != CANNON_TARGET_COUNT; ++index) {
        if (cannon_targets[index].x == cannon_shot_x &&
            cannon_targets[index].y == cannon_shot_y) {
            cannon_blank(cannon_targets[index].x, cannon_targets[index].y);
            ++cannon_score;
            cannon_draw_status();
            cannon_spawn_target(index, CANNON_FIELD_Y);
            cannon_draw_target(index);
            audio_sfx(SFX_CLICK);
            if (cannon_score >= CANNON_WIN_SCORE) cannon_finish(CANNON_WON);
            return 1u;
        }
    }
    return 0u;
}

static void cannon_update_shot(void)
{
    if (!cannon_shot_active || cannon_state != CANNON_PLAYING) return;

    ++cannon_shot_wait;
    if (cannon_shot_wait < CANNON_SHOT_PERIOD) return;
    cannon_shot_wait = 0u;

    cannon_blank(cannon_shot_x, cannon_shot_y);
    if (cannon_shot_y <= CANNON_FIELD_Y) {
        cannon_shot_active = 0u;
        return;
    }

    --cannon_shot_y;
    if (cannon_hit_target()) {
        cannon_shot_active = 0u;
        return;
    }

    ui_set_tile(cannon_shot_x, cannon_shot_y, TILE_CURSOR_MARK, PAL_MONO);
}

static void cannon_update_aim(const InputState *input)
{
    const PointerState *pointer;
    UINT8 aim_x;
    UINT8 aim_y;
    UINT8 next_cannon_x;

    pointer_update(input);
    pointer = pointer_get();
    aim_x = pointer->x;
    aim_y = pointer->y;

    if (aim_x < 16u) aim_x = 16u;
    else if (aim_x > 136u) aim_x = 136u;
    if (aim_y < (UINT8)(CANNON_FIELD_Y * 8u)) aim_y = (UINT8)(CANNON_FIELD_Y * 8u);
    else if (aim_y > 104u) aim_y = 104u;
    pointer_move_to(aim_x, aim_y);

    next_cannon_x = (UINT8)((aim_x + 4u) >> 3u);

    if (next_cannon_x != cannon_x) {
        cannon_blank(cannon_x, CANNON_ROW);
        cannon_x = next_cannon_x;
        ui_set_tile(cannon_x, CANNON_ROW, TILE_CANNON, PAL_MONO);
    }
}

static void cannon_fire(void)
{
    if (cannon_shot_active) return;

    cannon_shot_active = 1u;
    cannon_shot_x = cannon_x;
    cannon_shot_y = (UINT8)(CANNON_ROW - 1u);
    cannon_shot_wait = 0u;
    ui_set_tile(cannon_shot_x, cannon_shot_y, TILE_CURSOR_MARK, PAL_MONO);
    audio_sfx(SFX_SHOT);

    if (cannon_hit_target()) cannon_shot_active = 0u;
}

void cannon_enter(void) BANKED
{
    ui_scene_begin();
    pointer_hide();
    set_bkg_palette(PAL_APP_C, 1u, cannon_led_palette);
    ui_led_load(CANNON_ART_LED);
    ui_clear(PAL_DESKTOP);
    ui_window(0u, 0u, 20u, 18u, "Cannon", 1u, UI_CLIENT_FACE);
    ui_menu(1u, 1u, 18u, "Game  Help");
    ui_sunken(CANNON_FIELD_X, CANNON_FIELD_Y, CANNON_FIELD_W, CANNON_FIELD_H);
    ui_label_init(&lives_label, 12u, CANNON_HEADER_Y, 6u, PAL_WINDOW);
    ui_label(6u, CANNON_HEADER_Y, 6u, "Score", PAL_WINDOW,
             TEXT_COLORS(COLOR_GREY, COLOR_BLACK, 0u), 0u);
    ui_label(2u, 16u, 16u, "A fire   B reset   Start close", PAL_WINDOW,
             TEXT_COLORS(COLOR_GREY, COLOR_BLACK, 0u), 0u);
    ui_label_init(&message_label, 4u, 9u, 12u, PAL_MONO);
    ui_label_init(&restart_label, 4u, 11u, 12u, PAL_MONO);
    cannon_reset_game();
    ui_scene_end();
}

AppState cannon_update(const InputState *input) BANKED
{
    if (input->pressed & J_START) {
        pointer_hide();
        return APP_DESKTOP;
    }

    if (cannon_state != CANNON_PLAYING) {
        if (input->pressed & (J_A | J_B)) cannon_reset_game();
        return APP_CANNON;
    }

    if (input->pressed & J_B) {
        cannon_reset_game();
        return APP_CANNON;
    }

    cannon_update_aim(input);
    if (input->pressed & J_A) cannon_fire();

    ++cannon_target_wait;
    if (cannon_target_wait >= CANNON_TARGET_PERIOD) {
        cannon_target_wait = 0u;
        cannon_move_targets();
    }
    cannon_update_shot();

    return APP_CANNON;
}
