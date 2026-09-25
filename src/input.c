#include "input.h"

static InputState input_state;
static UINT8 previous_held;
static UINT8 repeat_frames;

/* Scene redraws can take longer than a frame, so a quick tap may start and
 * end between two input_update() calls. The VBlank handler latches press
 * edges so every tap is delivered exactly once. */
static volatile UINT8 latched_pressed;
static UINT8 vbl_previous;

static void input_vbl(void)
{
    UINT8 held = joypad();
    latched_pressed |= (UINT8)(held & (UINT8)~vbl_previous);
    vbl_previous = held;
}

void input_init(void)
{
    input_state.held = 0u;
    input_state.pressed = 0u;
    input_state.released = 0u;
    input_state.repeated = 0u;
    previous_held = 0u;
    repeat_frames = 0u;
    latched_pressed = 0u;
    vbl_previous = 0u;
    CRITICAL {
        add_VBL(input_vbl);
    }
}

void input_update(void)
{
    UINT8 directions;
    UINT8 latched;

    CRITICAL {
        latched = latched_pressed;
        latched_pressed = 0u;
    }

    input_state.held = joypad();
    input_state.pressed = (UINT8)((input_state.held & (UINT8)~previous_held) | latched);
    input_state.released = (UINT8)(previous_held & (UINT8)~input_state.held);
    input_state.repeated = input_state.pressed;

    directions = (UINT8)(input_state.held & (J_UP | J_DOWN | J_LEFT | J_RIGHT));
    if (directions == 0u) {
        repeat_frames = 0u;
    } else if ((input_state.pressed & directions) != 0u) {
        repeat_frames = 0u;
    } else {
        if (repeat_frames < 16u) {
            ++repeat_frames;
        } else {
            ++repeat_frames;
            if (repeat_frames > 19u) repeat_frames = 16u;
        }
        if (repeat_frames == 16u) {
            input_state.repeated |= directions;
        }
    }

    previous_held = input_state.held;
}

const InputState *input_get(void)
{
    return &input_state;
}
