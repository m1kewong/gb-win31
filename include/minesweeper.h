#ifndef GBW_MINESWEEPER_H
#define GBW_MINESWEEPER_H

#include "app.h"
#include "input.h"
#include "minesweeper_model.h"

#define MS_BOARD_TILE_X 5u
#define MS_BOARD_TILE_Y 5u

void minesweeper_enter(void);
AppState minesweeper_update(const InputState *input);

#endif
