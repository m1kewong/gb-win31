#ifndef MINESWEEPER_MODEL_H
#define MINESWEEPER_MODEL_H

#define MS_MODEL_WIDTH       10u
#define MS_MODEL_HEIGHT       8u
#define MS_MODEL_MINE_COUNT  10u
#define MS_MODEL_CELL_COUNT  (MS_MODEL_WIDTH * MS_MODEL_HEIGHT)

#define MS_MODEL_PLAYING 0u
#define MS_MODEL_WON     1u
#define MS_MODEL_LOST    2u

#define MS_MODEL_ACTION_NONE    0u
#define MS_MODEL_ACTION_CHANGED 1u
#define MS_MODEL_ACTION_WON     2u
#define MS_MODEL_ACTION_LOST    3u

typedef struct MinesweeperModelCell {
    unsigned char is_mine;
    unsigned char is_revealed;
    unsigned char is_flagged;
    unsigned char adjacent_mines;
} MinesweeperModelCell;

typedef struct MinesweeperModel {
    MinesweeperModelCell cells[MS_MODEL_CELL_COUNT];
    unsigned int rng_state;
    unsigned char status;
    unsigned char mines_placed;
    unsigned char flag_count;
    unsigned char revealed_safe_count;
} MinesweeperModel;

void minesweeper_model_init(MinesweeperModel *model, unsigned int seed);

unsigned char minesweeper_model_reveal(
    MinesweeperModel *model,
    unsigned char x,
    unsigned char y
);

unsigned char minesweeper_model_toggle_flag(
    MinesweeperModel *model,
    unsigned char x,
    unsigned char y
);

const MinesweeperModelCell *minesweeper_model_get_cell(
    const MinesweeperModel *model,
    unsigned char x,
    unsigned char y
);

unsigned char minesweeper_model_get_status(const MinesweeperModel *model);
unsigned char minesweeper_model_get_flag_count(const MinesweeperModel *model);
unsigned char minesweeper_model_get_revealed_safe_count(
    const MinesweeperModel *model
);
unsigned char minesweeper_model_are_mines_placed(
    const MinesweeperModel *model
);
unsigned char minesweeper_model_is_won(const MinesweeperModel *model);
unsigned char minesweeper_model_is_lost(const MinesweeperModel *model);

#endif
