#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "minesweeper_model.h"

static unsigned char test_count_mines(const MinesweeperModel *model)
{
    unsigned char index;
    unsigned char count;

    count = 0u;
    for (index = 0u; index < MS_MODEL_CELL_COUNT; ++index) {
        if (model->cells[index].is_mine) {
            ++count;
        }
    }
    return count;
}

static unsigned char test_expected_adjacency(
    const MinesweeperModel *model,
    unsigned char x,
    unsigned char y
)
{
    int neighbor_x;
    int neighbor_y;
    int offset_x;
    int offset_y;
    unsigned char count;
    const MinesweeperModelCell *cell;

    count = 0u;
    for (offset_y = -1; offset_y <= 1; ++offset_y) {
        for (offset_x = -1; offset_x <= 1; ++offset_x) {
            if (offset_x == 0 && offset_y == 0) {
                continue;
            }
            neighbor_x = (int)x + offset_x;
            neighbor_y = (int)y + offset_y;
            if (neighbor_x < 0 || neighbor_y < 0 ||
                neighbor_x >= (int)MS_MODEL_WIDTH ||
                neighbor_y >= (int)MS_MODEL_HEIGHT) {
                continue;
            }
            cell = minesweeper_model_get_cell(
                model,
                (unsigned char)neighbor_x,
                (unsigned char)neighbor_y
            );
            assert(cell != (const MinesweeperModelCell *)0);
            if (cell->is_mine) {
                ++count;
            }
        }
    }
    return count;
}

static void test_initial_state(void)
{
    MinesweeperModel model;
    unsigned char index;

    minesweeper_model_init(&model, 0x1234u);
    assert(minesweeper_model_get_status(&model) == MS_MODEL_PLAYING);
    assert(!minesweeper_model_are_mines_placed(&model));
    assert(minesweeper_model_get_flag_count(&model) == 0u);
    assert(minesweeper_model_get_revealed_safe_count(&model) == 0u);

    for (index = 0u; index < MS_MODEL_CELL_COUNT; ++index) {
        assert(!model.cells[index].is_mine);
        assert(!model.cells[index].is_revealed);
        assert(!model.cells[index].is_flagged);
        assert(model.cells[index].adjacent_mines == 0u);
    }
}

static void test_first_reveal_is_safe(void)
{
    MinesweeperModel model;
    unsigned char x;
    unsigned char y;
    unsigned int seed;
    const MinesweeperModelCell *cell;

    seed = 1u;
    for (y = 0u; y < MS_MODEL_HEIGHT; ++y) {
        for (x = 0u; x < MS_MODEL_WIDTH; ++x) {
            minesweeper_model_init(&model, seed);
            assert(minesweeper_model_reveal(&model, x, y) !=
                   MS_MODEL_ACTION_LOST);
            cell = minesweeper_model_get_cell(&model, x, y);
            assert(cell != (const MinesweeperModelCell *)0);
            assert(!cell->is_mine);
            assert(cell->is_revealed);
            assert(test_count_mines(&model) == MS_MODEL_MINE_COUNT);
            ++seed;
        }
    }
}

static void test_mine_count_and_adjacency(void)
{
    MinesweeperModel model;
    unsigned char x;
    unsigned char y;
    const MinesweeperModelCell *cell;

    minesweeper_model_init(&model, 0x1234u);
    minesweeper_model_reveal(&model, 5u, 4u);
    assert(test_count_mines(&model) == MS_MODEL_MINE_COUNT);

    for (y = 0u; y < MS_MODEL_HEIGHT; ++y) {
        for (x = 0u; x < MS_MODEL_WIDTH; ++x) {
            cell = minesweeper_model_get_cell(&model, x, y);
            assert(cell != (const MinesweeperModelCell *)0);
            if (!cell->is_mine) {
                assert(cell->adjacent_mines ==
                       test_expected_adjacency(&model, x, y));
                assert(cell->adjacent_mines <= 8u);
            }
        }
    }
}

static void test_determinism(void)
{
    static const unsigned char expected_mines[MS_MODEL_MINE_COUNT] = {
        7u, 17u, 28u, 33u, 38u, 46u, 48u, 62u, 65u, 74u
    };
    MinesweeperModel first;
    MinesweeperModel second;
    unsigned char index;
    unsigned char mine_number;

    minesweeper_model_init(&first, 0xBEEFu);
    minesweeper_model_init(&second, 0xBEEFu);
    minesweeper_model_reveal(&first, 3u, 6u);
    minesweeper_model_reveal(&second, 3u, 6u);

    assert(first.rng_state == second.rng_state);
    assert(first.status == second.status);
    assert(first.revealed_safe_count == second.revealed_safe_count);
    mine_number = 0u;
    for (index = 0u; index < MS_MODEL_CELL_COUNT; ++index) {
        if (first.cells[index].is_mine) {
            assert(mine_number < MS_MODEL_MINE_COUNT);
            assert(index == expected_mines[mine_number]);
            ++mine_number;
        }
        assert(first.cells[index].is_mine == second.cells[index].is_mine);
        assert(first.cells[index].is_revealed ==
               second.cells[index].is_revealed);
        assert(first.cells[index].is_flagged ==
               second.cells[index].is_flagged);
        assert(first.cells[index].adjacent_mines ==
               second.cells[index].adjacent_mines);
    }
    assert(mine_number == MS_MODEL_MINE_COUNT);
}

static void test_flags(void)
{
    MinesweeperModel model;
    unsigned char x;
    unsigned char scan_x;
    unsigned char scan_y;
    unsigned char hidden_x;
    unsigned char hidden_y;
    unsigned char found_hidden;
    const MinesweeperModelCell *cell;

    minesweeper_model_init(&model, 77u);
    assert(minesweeper_model_toggle_flag(&model, 1u, 1u) ==
           MS_MODEL_ACTION_CHANGED);
    assert(minesweeper_model_get_flag_count(&model) == 1u);
    assert(minesweeper_model_reveal(&model, 1u, 1u) ==
           MS_MODEL_ACTION_NONE);
    assert(!minesweeper_model_are_mines_placed(&model));
    assert(minesweeper_model_toggle_flag(&model, 1u, 1u) ==
           MS_MODEL_ACTION_CHANGED);
    assert(minesweeper_model_get_flag_count(&model) == 0u);

    for (x = 0u; x < MS_MODEL_MINE_COUNT; ++x) {
        assert(minesweeper_model_toggle_flag(&model, x, 0u) ==
               MS_MODEL_ACTION_CHANGED);
    }
    assert(minesweeper_model_get_flag_count(&model) ==
           MS_MODEL_MINE_COUNT);
    assert(minesweeper_model_toggle_flag(&model, 0u, 1u) ==
           MS_MODEL_ACTION_NONE);
    assert(minesweeper_model_toggle_flag(&model, 0u, 0u) ==
           MS_MODEL_ACTION_CHANGED);
    assert(minesweeper_model_toggle_flag(&model, 0u, 1u) ==
           MS_MODEL_ACTION_CHANGED);

    minesweeper_model_init(&model, 77u);
    minesweeper_model_reveal(&model, 5u, 4u);
    assert(minesweeper_model_toggle_flag(&model, 5u, 4u) ==
           MS_MODEL_ACTION_NONE);

    found_hidden = 0u;
    hidden_x = 0u;
    hidden_y = 0u;
    for (scan_y = 0u; scan_y < MS_MODEL_HEIGHT && !found_hidden;
         ++scan_y) {
        for (scan_x = 0u; scan_x < MS_MODEL_WIDTH; ++scan_x) {
            cell = minesweeper_model_get_cell(&model, scan_x, scan_y);
            if (cell != (const MinesweeperModelCell *)0 &&
                !cell->is_revealed) {
                hidden_x = scan_x;
                hidden_y = scan_y;
                found_hidden = 1u;
                break;
            }
        }
    }
    assert(found_hidden);
    assert(minesweeper_model_toggle_flag(&model, hidden_x, hidden_y) ==
           MS_MODEL_ACTION_CHANGED);
    assert(minesweeper_model_reveal(&model, hidden_x, hidden_y) ==
           MS_MODEL_ACTION_NONE);
}

static void test_flood_fill_and_bounds(void)
{
    struct GuardedModel {
        unsigned char before[8];
        MinesweeperModel model;
        unsigned char after[8];
    } guarded;
    unsigned int seed;
    unsigned char found_zero;
    unsigned char index;
    unsigned char revealed_before;
    const MinesweeperModelCell *cell;

    found_zero = 0u;
    for (seed = 1u; seed < 500u; ++seed) {
        memset(&guarded, 0xA5, sizeof(guarded));
        minesweeper_model_init(&guarded.model, seed);
        minesweeper_model_reveal(&guarded.model, 5u, 4u);
        cell = minesweeper_model_get_cell(&guarded.model, 5u, 4u);
        assert(cell != (const MinesweeperModelCell *)0);
        if (cell->adjacent_mines == 0u) {
            found_zero = 1u;
            break;
        }
    }
    assert(found_zero);
    assert(minesweeper_model_get_revealed_safe_count(&guarded.model) > 1u);
    assert(minesweeper_model_get_revealed_safe_count(&guarded.model) <=
           (unsigned char)(MS_MODEL_CELL_COUNT - MS_MODEL_MINE_COUNT));

    for (index = 0u; index < 8u; ++index) {
        assert(guarded.before[index] == 0xA5u);
        assert(guarded.after[index] == 0xA5u);
    }
    for (index = 0u; index < MS_MODEL_CELL_COUNT; ++index) {
        if (guarded.model.cells[index].is_revealed) {
            assert(!guarded.model.cells[index].is_mine);
        }
    }

    revealed_before = minesweeper_model_get_revealed_safe_count(
        &guarded.model
    );
    assert(minesweeper_model_reveal(
               &guarded.model,
               MS_MODEL_WIDTH,
               0u
           ) == MS_MODEL_ACTION_NONE);
    assert(minesweeper_model_reveal(
               &guarded.model,
               0u,
               MS_MODEL_HEIGHT
           ) == MS_MODEL_ACTION_NONE);
    assert(minesweeper_model_toggle_flag(
               &guarded.model,
               MS_MODEL_WIDTH,
               MS_MODEL_HEIGHT
           ) == MS_MODEL_ACTION_NONE);
    assert(minesweeper_model_get_cell(
               &guarded.model,
               MS_MODEL_WIDTH,
               0u
           ) == (const MinesweeperModelCell *)0);
    assert(minesweeper_model_get_revealed_safe_count(&guarded.model) ==
           revealed_before);
}

static void test_loss(void)
{
    MinesweeperModel model;
    unsigned int seed;
    unsigned char index;
    unsigned char mine_index;
    unsigned char found_playing;

    found_playing = 0u;
    for (seed = 1u; seed < 100u; ++seed) {
        minesweeper_model_init(&model, seed);
        minesweeper_model_reveal(&model, 5u, 4u);
        if (minesweeper_model_get_status(&model) == MS_MODEL_PLAYING) {
            found_playing = 1u;
            break;
        }
    }
    assert(found_playing);

    mine_index = 0u;
    for (index = 0u; index < MS_MODEL_CELL_COUNT; ++index) {
        if (model.cells[index].is_mine) {
            mine_index = index;
            break;
        }
    }
    assert(model.cells[mine_index].is_mine);
    assert(minesweeper_model_reveal(
               &model,
               (unsigned char)(mine_index % MS_MODEL_WIDTH),
               (unsigned char)(mine_index / MS_MODEL_WIDTH)
           ) == MS_MODEL_ACTION_LOST);
    assert(minesweeper_model_is_lost(&model));
    assert(!minesweeper_model_is_won(&model));

    for (index = 0u; index < MS_MODEL_CELL_COUNT; ++index) {
        if (model.cells[index].is_mine) {
            assert(model.cells[index].is_revealed);
        }
    }
    assert(minesweeper_model_toggle_flag(&model, 0u, 0u) ==
           MS_MODEL_ACTION_NONE);
}

static void test_win(void)
{
    MinesweeperModel model;
    unsigned char x;
    unsigned char y;
    const MinesweeperModelCell *cell;

    minesweeper_model_init(&model, 321u);
    minesweeper_model_reveal(&model, 5u, 4u);

    for (y = 0u; y < MS_MODEL_HEIGHT; ++y) {
        for (x = 0u; x < MS_MODEL_WIDTH; ++x) {
            cell = minesweeper_model_get_cell(&model, x, y);
            assert(cell != (const MinesweeperModelCell *)0);
            if (!cell->is_mine && !cell->is_revealed) {
                minesweeper_model_reveal(&model, x, y);
            }
        }
    }

    assert(minesweeper_model_is_won(&model));
    assert(!minesweeper_model_is_lost(&model));
    assert(minesweeper_model_get_status(&model) == MS_MODEL_WON);
    assert(minesweeper_model_get_revealed_safe_count(&model) ==
           (unsigned char)(MS_MODEL_CELL_COUNT - MS_MODEL_MINE_COUNT));
    assert(minesweeper_model_reveal(&model, 0u, 0u) ==
           MS_MODEL_ACTION_NONE);
}

int main(void)
{
    test_initial_state();
    test_first_reveal_is_safe();
    test_mine_count_and_adjacency();
    test_determinism();
    test_flags();
    test_flood_fill_and_bounds();
    test_loss();
    test_win();

    puts("minesweeper_model: all tests passed");
    return 0;
}
