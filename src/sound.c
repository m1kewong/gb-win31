#include <gb/gb.h>
#include "sound.h"

// Initialize sound hardware
void init_sound(void) {
    // Turn on sound hardware
    NR52_REG = 0x80; // Bit 7: All sound on/off (1=on, 0=off)
    // Set master volume for SO1 & SO2 (left and right channels)
    NR50_REG = 0x77; // Bits 6-4: SO2 output level (volume), Bits 2-0: SO1 output level (volume)
    // Select sound channels for SO1 & SO2
    NR51_REG = 0xFF; // Bit 7: Ch4 to SO2, Bit 6: Ch3 to SO2, Bit 5: Ch2 to SO2, Bit 4: Ch1 to SO2
                     // Bit 3: Ch4 to SO1, Bit 2: Ch3 to SO1, Bit 1: Ch2 to SO1, Bit 0: Ch1 to SO1
}

void play_chime(void) {
    // Ensure sound is initialized
    // init_sound(); // Call this once at the start of your game, e.g., in main()

    // Channel 1: Tone & Sweep
    NR10_REG = 0x1E; // Sweep: time 1, direction increase, shift 6 (Moderate sweep up)
    NR11_REG = 0x80; // Duty 50% (0x80), Sound length 0 (continuous until stop or retrigger)
    NR12_REG = 0xF3; // Envelope: Initial Vol 15, Direction Increase, Step Time 3
    NR13_REG = 0x85; // Frequency Lo (Combined with NR14 Hi bits for full frequency)
                     // Example: Freq = 2048 - ( (NR14_Hi << 8) | NR13_Lo ) -> 2048 - ( (0x00 << 8) | 0x85) = 2048 - 133 = 1915 Hz approx
                     // Let's use a higher note for a chime, e.g. ~1000Hz. Freq data = 2048 - 1000 = 1048 = 0x418
    NR13_REG = 0x18; // Freq Lo for ~1000Hz
    NR14_REG = 0x84; // Trigger (1), Length Enable (0=continuous), Freq Hi (0x04 for ~1000Hz)

    // A short delay or managing sound length via NR11 is needed for distinct notes
    // For a simple chime, one trigger might be enough, or play a sequence.
}

void play_sound(UINT8 sfx_id) {
    // Ensure sound is initialized
    // init_sound(); // Call this once at the start of your game

    // Play sound based on sfx_id
    switch(sfx_id) {
        case SFX_BOOT:
            NR10_REG = 0x1E; NR11_REG = 0x10; NR12_REG = 0xF3; NR13_REG = 0x00; NR14_REG = 0x87;
            break;
        case SFX_CURSOR_MOVE:
            NR10_REG = 0x00; NR11_REG = 0x81; NR12_REG = 0x43; NR13_REG = 0x73; NR14_REG = 0x86;
            break;
        case SFX_SELECT: 
            NR10_REG = 0x3D; NR11_REG = 0x4A; NR12_REG = 0xA7; NR13_REG = 0x7F; NR14_REG = 0x86;
            break;
        case SFX_REVEAL_EMPTY:
            NR10_REG = 0x00; NR11_REG = 0x81; NR12_REG = 0x43; NR13_REG = 0x73; NR14_REG = 0x86; // Same as cursor move for now
            break;
        case SFX_PLACE_FLAG:
            NR10_REG = 0x78; NR11_REG = 0x64; NR12_REG = 0x82; NR13_REG = 0x28; NR14_REG = 0x86;
            break;
        case SFX_REVEAL_MINE: // SFX_LOSE_GAME is an alias for this
            NR10_REG = 0x46; NR11_REG = 0xA9; NR12_REG = 0x0F; NR13_REG = 0x7C; NR14_REG = 0x87;
            break;
        case SFX_WIN_GAME:
            NR10_REG = 0x1E; NR11_REG = 0x10; NR12_REG = 0xF3; NR13_REG = 0x00; NR14_REG = 0x87; // Same as boot for now
            break;
        case SFX_PAINT_DRAW:
            NR10_REG = 0x00; NR11_REG = 0x10; NR12_REG = 0xA0; NR13_REG = 0x05; NR14_REG = 0x86;
            break;
        case SFX_PAINT_TOOL_SELECT:
            NR10_REG = 0x3D; NR11_REG = 0x4A; NR12_REG = 0xA7; NR13_REG = 0x7F; NR14_REG = 0x86; // Same as select
            break;
        case SFX_PAINT_ERASE:
            NR10_REG = 0x00; NR11_REG = 0x10; NR12_REG = 0xA0; NR13_REG = 0x05; NR14_REG = 0x86; // Same as draw for now
            break;
        default:
            break;
    }
}