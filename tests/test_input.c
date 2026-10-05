#include <assert.h>
#include <stdio.h>

#include "input.h"

static UINT8 pad;
static int_handler vblank_handler;

UINT8 joypad(void)
{
    return pad;
}

void add_VBL(int_handler handler)
{
    vblank_handler = handler;
}

static void vblank(void)
{
    vblank_handler();
}

/* One normal frame: the VBlank interrupt samples, then the main loop updates. */
static UINT8 frame(void)
{
    vblank();
    input_update();
    return input_get()->pressed;
}

static void reset(void)
{
    pad = 0u;
    input_init();
    assert(vblank_handler != (int_handler)0);
    frame();
}

static void test_press_between_vblank_and_update_counts_once(void)
{
    unsigned count = 0u;
    unsigned i;

    reset();
    vblank();
    pad = J_A;
    input_update();
    if (input_get()->pressed & J_A) ++count;
    for (i = 0u; i != 5u; ++i) {
        if (frame() & J_A) ++count;
    }
    assert(count == 1u);
}

static void test_tap_inside_a_long_frame_is_delivered_once(void)
{
    reset();
    vblank();
    pad = J_A;
    vblank();
    pad = 0u;
    vblank();
    input_update();
    assert(input_get()->pressed & J_A);
    assert(!(input_get()->held & J_A));
    assert(!(frame() & J_A));
}

static void test_press_seen_by_vblank_and_update_counts_once(void)
{
    unsigned count = 0u;
    unsigned i;

    reset();
    pad = J_B;
    for (i = 0u; i != 5u; ++i) {
        if (frame() & J_B) ++count;
    }
    assert(count == 1u);
}

static void test_two_taps_count_twice(void)
{
    unsigned count = 0u;

    reset();
    pad = J_A;
    if (frame() & J_A) ++count;
    pad = 0u;
    if (frame() & J_A) ++count;
    pad = J_A;
    if (frame() & J_A) ++count;
    assert(count == 2u);
}

static void test_held_direction_repeats(void)
{
    unsigned repeats = 0u;
    unsigned i;

    reset();
    pad = J_LEFT;
    assert(frame() & J_LEFT);
    for (i = 0u; i != 30u; ++i) {
        frame();
        if (input_get()->repeated & J_LEFT) ++repeats;
        assert(!(input_get()->pressed & J_LEFT));
    }
    assert(repeats >= 2u);
}

int main(void)
{
    test_press_between_vblank_and_update_counts_once();
    test_tap_inside_a_long_frame_is_delivered_once();
    test_press_seen_by_vblank_and_update_counts_once();
    test_two_taps_count_twice();
    test_held_direction_repeats();
    puts("input: all tests passed");
    return 0;
}
