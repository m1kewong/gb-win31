#include "minesweeper_model.h"

static unsigned char ms_model_in_bounds(unsigned char x, unsigned char y)
{
    return (unsigned char)(x < MS_MODEL_WIDTH && y < MS_MODEL_HEIGHT);
}

static unsigned char ms_model_index(unsigned char x, unsigned char y)
{
    return (unsigned char)(y * MS_MODEL_WIDTH + x);
}

static unsigned int ms_model_next_random(MinesweeperModel *model)
{
    model->rng_state = (unsigned int)(
        (model->rng_state * 25173u + 13849u) & 65535u
    );
    return model->rng_state;
}

static void ms_model_calculate_adjacency(MinesweeperModel *model)
{
    int x;
    int y;
    int neighbor_x;
    int neighbor_y;
    int offset_x;
    int offset_y;
    unsigned char count;
    unsigned char index;
    unsigned char neighbor_index;

    for (y = 0; y < (int)MS_MODEL_HEIGHT; ++y) {
        for (x = 0; x < (int)MS_MODEL_WIDTH; ++x) {
            index = ms_model_index((unsigned char)x, (unsigned char)y);
            count = 0u;

            if (model->cells[index].is_mine) {
                model->cells[index].adjacent_mines = 0u;
                continue;
            }

            for (offset_y = -1; offset_y <= 1; ++offset_y) {
                for (offset_x = -1; offset_x <= 1; ++offset_x) {
                    if (offset_x == 0 && offset_y == 0) {
                        continue;
                    }

                    neighbor_x = x + offset_x;
                    neighbor_y = y + offset_y;
                    if (neighbor_x < 0 || neighbor_y < 0 ||
                        neighbor_x >= (int)MS_MODEL_WIDTH ||
                        neighbor_y >= (int)MS_MODEL_HEIGHT) {
                        continue;
                    }

                    neighbor_index = ms_model_index(
                        (unsigned char)neighbor_x,
                        (unsigned char)neighbor_y
                    );
                    if (model->cells[neighbor_index].is_mine) {
                        ++count;
                    }
                }
            }

            model->cells[index].adjacent_mines = count;
        }
    }
}

static void ms_model_place_mines(
    MinesweeperModel *model,
    unsigned char safe_index
)
{
    unsigned char candidates[MS_MODEL_CELL_COUNT - 1u];
    unsigned char candidate_count;
    unsigned char cell_index;
    unsigned char mine_index;
    unsigned char swap_index;
    unsigned char temporary;
    unsigned char remaining;

    candidate_count = 0u;
    for (cell_index = 0u; cell_index < MS_MODEL_CELL_COUNT; ++cell_index) {
        if (cell_index != safe_index) {
            candidates[candidate_count] = cell_index;
            ++candidate_count;
        }
    }

    for (mine_index = 0u; mine_index < MS_MODEL_MINE_COUNT; ++mine_index) {
        remaining = (unsigned char)(candidate_count - mine_index);
        swap_index = (unsigned char)(
            mine_index +
            (unsigned char)(ms_model_next_random(model) % remaining)
        );

        temporary = candidates[mine_index];
        candidates[mine_index] = candidates[swap_index];
        candidates[swap_index] = temporary;
        model->cells[candidates[mine_index]].is_mine = 1u;
    }

    model->mines_placed = 1u;
    ms_model_calculate_adjacency(model);
}

static void ms_model_reveal_all_mines(MinesweeperModel *model)
{
    unsigned char index;

    for (index = 0u; index < MS_MODEL_CELL_COUNT; ++index) {
        if (model->cells[index].is_mine) {
            model->cells[index].is_revealed = 1u;
        }
    }
}

void minesweeper_model_init(MinesweeperModel *model, unsigned int seed)
{
    unsigned char index;

    if (model == (MinesweeperModel *)0) {
        return;
    }

    for (index = 0u; index < MS_MODEL_CELL_COUNT; ++index) {
        model->cells[index].is_mine = 0u;
        model->cells[index].is_revealed = 0u;
        model->cells[index].is_flagged = 0u;
        model->cells[index].adjacent_mines = 0u;
    }

    model->rng_state = (unsigned int)(seed & 65535u);
    model->status = MS_MODEL_PLAYING;
    model->mines_placed = 0u;
    model->flag_count = 0u;
    model->revealed_safe_count = 0u;
}

unsigned char minesweeper_model_reveal(
    MinesweeperModel *model,
    unsigned char x,
    unsigned char y
)
{
    unsigned char queue[MS_MODEL_CELL_COUNT];
    unsigned char queue_head;
    unsigned char queue_tail;
    unsigned char index;
    unsigned char current_index;
    unsigned char current_x;
    unsigned char current_y;
    int neighbor_x;
    int neighbor_y;
    int offset_x;
    int offset_y;
    unsigned char neighbor_index;
    MinesweeperModelCell *cell;
    MinesweeperModelCell *neighbor;

    if (model == (MinesweeperModel *)0 ||
        model->status != MS_MODEL_PLAYING ||
        !ms_model_in_bounds(x, y)) {
        return MS_MODEL_ACTION_NONE;
    }

    index = ms_model_index(x, y);
    cell = &model->cells[index];
    if (cell->is_revealed || cell->is_flagged) {
        return MS_MODEL_ACTION_NONE;
    }

    if (!model->mines_placed) {
        ms_model_place_mines(model, index);
    }

    if (cell->is_mine) {
        ms_model_reveal_all_mines(model);
        model->status = MS_MODEL_LOST;
        return MS_MODEL_ACTION_LOST;
    }

    queue_head = 0u;
    queue_tail = 0u;
    cell->is_revealed = 1u;
    ++model->revealed_safe_count;
    if (cell->adjacent_mines == 0u) {
        queue[queue_tail] = index;
        ++queue_tail;
    }

    while (queue_head < queue_tail) {
        current_index = queue[queue_head];
        ++queue_head;
        current_x = (unsigned char)(current_index % MS_MODEL_WIDTH);
        current_y = (unsigned char)(current_index / MS_MODEL_WIDTH);

        for (offset_y = -1; offset_y <= 1; ++offset_y) {
            for (offset_x = -1; offset_x <= 1; ++offset_x) {
                if (offset_x == 0 && offset_y == 0) {
                    continue;
                }

                neighbor_x = (int)current_x + offset_x;
                neighbor_y = (int)current_y + offset_y;
                if (neighbor_x < 0 || neighbor_y < 0 ||
                    neighbor_x >= (int)MS_MODEL_WIDTH ||
                    neighbor_y >= (int)MS_MODEL_HEIGHT) {
                    continue;
                }

                neighbor_index = ms_model_index(
                    (unsigned char)neighbor_x,
                    (unsigned char)neighbor_y
                );
                neighbor = &model->cells[neighbor_index];
                if (neighbor->is_mine || neighbor->is_flagged ||
                    neighbor->is_revealed) {
                    continue;
                }

                neighbor->is_revealed = 1u;
                ++model->revealed_safe_count;
                if (neighbor->adjacent_mines == 0u) {
                    queue[queue_tail] = neighbor_index;
                    ++queue_tail;
                }
            }
        }
    }

    if (model->revealed_safe_count ==
        (unsigned char)(MS_MODEL_CELL_COUNT - MS_MODEL_MINE_COUNT)) {
        model->status = MS_MODEL_WON;
        return MS_MODEL_ACTION_WON;
    }

    return MS_MODEL_ACTION_CHANGED;
}

unsigned char minesweeper_model_toggle_flag(
    MinesweeperModel *model,
    unsigned char x,
    unsigned char y
)
{
    MinesweeperModelCell *cell;

    if (model == (MinesweeperModel *)0 ||
        model->status != MS_MODEL_PLAYING ||
        !ms_model_in_bounds(x, y)) {
        return MS_MODEL_ACTION_NONE;
    }

    cell = &model->cells[ms_model_index(x, y)];
    if (cell->is_revealed) {
        return MS_MODEL_ACTION_NONE;
    }

    if (cell->is_flagged) {
        cell->is_flagged = 0u;
        --model->flag_count;
        return MS_MODEL_ACTION_CHANGED;
    }

    if (model->flag_count >= MS_MODEL_MINE_COUNT) {
        return MS_MODEL_ACTION_NONE;
    }

    cell->is_flagged = 1u;
    ++model->flag_count;
    return MS_MODEL_ACTION_CHANGED;
}

const MinesweeperModelCell *minesweeper_model_get_cell(
    const MinesweeperModel *model,
    unsigned char x,
    unsigned char y
)
{
    if (model == (const MinesweeperModel *)0 ||
        !ms_model_in_bounds(x, y)) {
        return (const MinesweeperModelCell *)0;
    }

    return &model->cells[ms_model_index(x, y)];
}

unsigned char minesweeper_model_get_status(const MinesweeperModel *model)
{
    if (model == (const MinesweeperModel *)0) {
        return MS_MODEL_LOST;
    }
    return model->status;
}

unsigned char minesweeper_model_get_flag_count(const MinesweeperModel *model)
{
    if (model == (const MinesweeperModel *)0) {
        return 0u;
    }
    return model->flag_count;
}

unsigned char minesweeper_model_get_revealed_safe_count(
    const MinesweeperModel *model
)
{
    if (model == (const MinesweeperModel *)0) {
        return 0u;
    }
    return model->revealed_safe_count;
}

unsigned char minesweeper_model_are_mines_placed(
    const MinesweeperModel *model
)
{
    if (model == (const MinesweeperModel *)0) {
        return 0u;
    }
    return model->mines_placed;
}

unsigned char minesweeper_model_is_won(const MinesweeperModel *model)
{
    return (unsigned char)(
        model != (const MinesweeperModel *)0 && model->status == MS_MODEL_WON
    );
}

unsigned char minesweeper_model_is_lost(const MinesweeperModel *model)
{
    return (unsigned char)(
        model != (const MinesweeperModel *)0 && model->status == MS_MODEL_LOST
    );
}
