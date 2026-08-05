#include <gb/gb.h>

#include "audio.h"

static UINT8 music_track;
static UINT8 music_step;
static UINT8 music_wait;

static const UINT16 note_frequency[] = {
    1547u, 1602u, 1650u, 1673u, 1714u, 1750u, 1783u, 1798u,
    1827u, 1850u, 1861u, 1882u, 1900u, 1916u, 1924u, 1935u
};

static const UINT8 track_one[] = {0u, 2u, 4u, 7u, 4u, 2u, 0u, 4u};
static const UINT8 track_two[] = {7u, 6u, 4u, 2u, 4u, 6u, 7u, 11u};
static const UINT8 track_three[] = {0u, 4u, 7u, 11u, 7u, 4u, 2u, 6u};
static const UINT8 track_four[] = {4u, 3u, 4u, 0u, 7u, 6u, 4u, 2u};

static void channel_two_note(UINT8 note)
{
    UINT16 frequency = note_frequency[note & 15u];
    NR21_REG = 0x80u;
    NR22_REG = 0x82u;
    NR23_REG = (UINT8)(frequency & 0xffu);
    NR24_REG = (UINT8)(0x80u | ((frequency >> 8u) & 7u));
}

void audio_init(void)
{
    NR52_REG = 0x80u;
    NR50_REG = 0x77u;
    NR51_REG = 0xffu;
    music_track = 0u;
    music_step = 0u;
    music_wait = 0u;
}

void audio_sfx(UINT8 effect)
{
    switch (effect) {
        case SFX_BOOT:
            NR10_REG = 0x16u; NR11_REG = 0x80u; NR12_REG = 0xf2u;
            NR13_REG = 0x40u; NR14_REG = 0x87u;
            break;
        case SFX_MOVE:
            NR10_REG = 0x00u; NR11_REG = 0x80u; NR12_REG = 0x42u;
            NR13_REG = 0x80u; NR14_REG = 0x86u;
            break;
        case SFX_CLICK:
            NR10_REG = 0x11u; NR11_REG = 0x40u; NR12_REG = 0x93u;
            NR13_REG = 0xc0u; NR14_REG = 0x86u;
            break;
        case SFX_ERROR:
            NR10_REG = 0x00u; NR11_REG = 0xc0u; NR12_REG = 0xa3u;
            NR13_REG = 0x20u; NR14_REG = 0x84u;
            break;
        case SFX_WIN:
            NR10_REG = 0x12u; NR11_REG = 0x80u; NR12_REG = 0xc4u;
            NR13_REG = 0xe0u; NR14_REG = 0x87u;
            break;
        case SFX_DRAW:
            NR10_REG = 0x00u; NR11_REG = 0x40u; NR12_REG = 0x31u;
            NR13_REG = 0x70u; NR14_REG = 0x85u;
            break;
        default:
            NR10_REG = 0x00u; NR11_REG = 0x40u; NR12_REG = 0x73u;
            NR13_REG = 0x10u; NR14_REG = 0x86u;
            break;
    }
}

void audio_note(UINT8 note, UINT8 duration, UINT8 duty)
{
    UINT16 frequency = note_frequency[note & 15u];
    NR10_REG = 0x00u;
    NR11_REG = (UINT8)((duty & 0xc0u) | (64u - (duration & 63u)));
    NR12_REG = 0xb2u;
    NR13_REG = (UINT8)(frequency & 0xffu);
    NR14_REG = (UINT8)(0xc0u | ((frequency >> 8u) & 7u));
}

void audio_music_set(UINT8 track)
{
    music_track = (track > 4u) ? 0u : track;
    music_step = 0u;
    music_wait = 0u;
    if (music_track == 0u) NR22_REG = 0u;
}

UINT8 audio_music_get(void)
{
    return music_track;
}

void audio_tick(void)
{
    const UINT8 *sequence;

    if (music_track == 0u) return;
    if (music_wait != 0u) {
        --music_wait;
        return;
    }

    if (music_track == 1u) sequence = track_one;
    else if (music_track == 2u) sequence = track_two;
    else if (music_track == 3u) sequence = track_three;
    else sequence = track_four;

    channel_two_note(sequence[music_step]);
    music_step = (UINT8)((music_step + 1u) & 7u);
    music_wait = 11u;
}
