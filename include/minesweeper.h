#ifndef GBW_MINESWEEPER_H
#define GBW_MINESWEEPER_H

#include <gb/gb.h>
#include "app.h"
#include "input.h"
#include "minesweeper_model.h"

#define MS_BOARD_TILE_X 5u
#define MS_BOARD_TILE_Y 8u

void minesweeper_enter(void) BANKED;
AppState minesweeper_update(const InputState *input) BANKED;

#endif
