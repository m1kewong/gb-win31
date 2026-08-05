#ifndef GBW_AUDIO_H
#define GBW_AUDIO_H

#include <gb/gb.h>

enum SoundEffect {
    SFX_BOOT = 0,
    SFX_MOVE,
    SFX_CLICK,
    SFX_ERROR,
    SFX_WIN,
    SFX_DRAW,
    SFX_SHOT
};

void audio_init(void);
void audio_tick(void);
void audio_sfx(UINT8 effect);
void audio_note(UINT8 note, UINT8 duration, UINT8 duty);
void audio_music_set(UINT8 track);
UINT8 audio_music_get(void);

#endif
