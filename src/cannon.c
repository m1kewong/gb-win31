#include <gb/gb.h>

#include "assets.h"
#include "audio.h"
#include "cannon.h"
#include "ui.h"

#define CANNON_FIELD_X 2u
#define CANNON_FIELD_Y 2u
#define CANNON_FIELD_W 16u
#define CANNON_FIELD_H 13u
#define CANNON_ROW 14u
#define CANNON_TARGET_BOTTOM 13u
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

static CannonTarget cannon_targets[CANNON_TARGET_COUNT];
static UINT8 cannon_x;
static UINT8 cannon_score;
static UINT8 cannon_lives;
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
    ui_text_clipped(2u, 1u, "SCORE:", PAL_WINDOW, 6u);
    ui_set_tile(8u, 1u,
                assets_font_tile((char)('0' + (cannon_score / 10u))), PAL_WINDOW);
    ui_set_tile(9u, 1u,
                assets_font_tile((char)('0' + (cannon_score % 10u))), PAL_WINDOW);
    ui_set_tile(10u, 1u, TILE_BLANK, PAL_WINDOW);
    ui_text_clipped(11u, 1u, "LIVES:", PAL_WINDOW, 6u);
    ui_set_tile(17u, 1u,
                assets_font_tile((char)('0' + cannon_lives)), PAL_WINDOW);
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

    if (result == CANNON_WON) {
        ui_text_clipped(5u, 7u, "CITY SAFE!", PAL_MONO, 10u);
        ui_text_clipped(5u, 9u, "A:RESTART", PAL_MONO, 10u);
        audio_sfx(SFX_WIN);
    } else {
        ui_text_clipped(4u, 7u, "DEFENSE DOWN", PAL_MONO, 12u);
        ui_text_clipped(5u, 9u, "A:RESTART", PAL_MONO, 10u);
        audio_sfx(SFX_ERROR);
    }
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

    cannon_spawn_target(0u, 3u);
    cannon_spawn_target(1u, 6u);
    cannon_spawn_target(2u, 9u);
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
    if (aim_y < 16u) aim_y = 16u;
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

void cannon_enter(void)
{
    ui_scene_begin();
    pointer_hide();
    ui_clear(PAL_DESKTOP);
    ui_window(1u, 0u, 18u, 18u, "CANNON", 1u);
    ui_text_clipped(2u, 15u, "A:FIRE  B:RESET", PAL_WINDOW, 16u);
    ui_text_clipped(2u, 16u, "START:DESKTOP", PAL_WINDOW, 16u);
    cannon_reset_game();
    ui_scene_end();
}

AppState cannon_update(const InputState *input)
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
