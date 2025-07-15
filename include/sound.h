#ifndef SOUND_H
#define SOUND_H

#include <gb/gb.h>

// Sound Effect IDs
#define SFX_BOOT 0
#define SFX_CURSOR_MOVE 1
#define SFX_SELECT 2          // General select/confirm sound
#define SFX_REVEAL_EMPTY 3    // Minesweeper: revealing an empty cell
#define SFX_REVEAL_MINE 4     // Minesweeper: revealing a mine (game over)
#define SFX_PLACE_FLAG 5      // Minesweeper: placing/removing a flag
#define SFX_WIN_GAME 6        // Minesweeper: winning the game
#define SFX_LOSE_GAME SFX_REVEAL_MINE // Alias for game over, as revealing mine is the primary lose sound
#define SFX_PAINT_DRAW 8
#define SFX_PAINT_ERASE 9
#define SFX_PAINT_TOOL_SELECT 10 // Paint: selecting a tool or color
// Add more SFX IDs as needed

void init_sound(void);
extern void play_sound(UINT8 sfx_id);

#endif // SOUND_H