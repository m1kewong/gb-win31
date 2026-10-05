#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "solitaire_model.h"

#define CLUBS 0u
#define DIAMONDS 1u
#define HEARTS 2u
#define SPADES 3u
#define CARD(suit, rank) ((unsigned char)((suit) * SOL_RANKS + (rank)))
#define T(column) ((unsigned char)(SOL_PILE_TABLEAU + (column)))
#define F(index) ((unsigned char)(SOL_PILE_FOUNDATION + (index)))

static void clear_model(SolitaireModel *model)
{
    solitaire_model_init(model, 1u, 1u);
    memset(model->tableau_count, 0, sizeof(model->tableau_count));
    memset(model->tableau_hidden, 0, sizeof(model->tableau_hidden));
    memset(model->foundation_count, 0, sizeof(model->foundation_count));
    model->stock_count = 0u;
    model->waste_count = 0u;
    model->score = 0u;
}

static void push_tableau(SolitaireModel *model, unsigned char column, unsigned char card)
{
    model->tableau[column][model->tableau_count[column]++] = card;
}

static void assert_invariants(const SolitaireModel *model)
{
    unsigned char seen[SOL_CARD_COUNT];
    unsigned char pile;
    unsigned char index;
    unsigned char size;
    unsigned char card;
    unsigned char below;
    unsigned char column;
    unsigned int total = 0u;

    memset(seen, 0, sizeof(seen));
    for (pile = 0u; pile != SOL_PILE_COUNT; ++pile) {
        size = solitaire_model_pile_size(model, pile);
        for (index = 0u; index != size; ++index) {
            card = solitaire_model_card_at(model, pile, index);
            assert(card < SOL_CARD_COUNT);
            assert(!seen[card]);
            seen[card] = 1u;
            ++total;
        }
    }
    assert(total == SOL_CARD_COUNT);

    for (column = 0u; column != SOL_TABLEAU_COUNT; ++column) {
        size = model->tableau_count[column];
        assert(size <= SOL_TABLEAU_MAX);
        assert(model->tableau_hidden[column] <= size);
        if (size != 0u) assert(model->tableau_hidden[column] < size);
        for (index = (unsigned char)(model->tableau_hidden[column] + 1u); index < size; ++index) {
            card = model->tableau[column][index];
            below = model->tableau[column][index - 1u];
            assert(SOL_RANK(below) == (unsigned char)(SOL_RANK(card) + 1u));
            assert(SOL_IS_RED(below) != SOL_IS_RED(card));
        }
    }
}

static void test_deal(void)
{
    SolitaireModel model;
    unsigned char column;

    solitaire_model_init(&model, 0x1234u, 1u);
    assert_invariants(&model);
    for (column = 0u; column != SOL_TABLEAU_COUNT; ++column) {
        assert(model.tableau_count[column] == column + 1u);
        assert(model.tableau_hidden[column] == column);
    }
    assert(model.stock_count == 24u);
    assert(model.waste_count == 0u);
    assert(solitaire_model_score(&model) == 0u);
    assert(!solitaire_model_is_won(&model));
    assert(model.draw_count == 1u);

    solitaire_model_init(&model, 0x1234u, 3u);
    assert(model.draw_count == 3u);
    solitaire_model_init(&model, 0x1234u, 2u);
    assert(model.draw_count == 1u);
}

static void test_determinism(void)
{
    SolitaireModel first;
    SolitaireModel second;
    unsigned int seed;
    unsigned char differs = 0u;

    solitaire_model_init(&first, 42u, 1u);
    solitaire_model_init(&second, 42u, 1u);
    assert(memcmp(&first, &second, sizeof(first)) == 0);

    for (seed = 43u; seed != 48u; ++seed) {
        solitaire_model_init(&second, seed, 1u);
        if (memcmp(first.stock, second.stock, sizeof(first.stock)) != 0) differs = 1u;
    }
    assert(differs);
}

static void test_draw_one_and_recycle(void)
{
    SolitaireModel model;
    unsigned char original[SOL_STOCK_MAX];
    unsigned char draw;

    solitaire_model_init(&model, 7u, 1u);
    memcpy(original, model.stock, sizeof(original));
    for (draw = 0u; draw != 24u; ++draw) {
        assert(solitaire_model_draw(&model) == SOL_ACTION_CHANGED);
        assert(solitaire_model_top(&model, SOL_PILE_WASTE) == original[23u - draw]);
    }
    assert(model.stock_count == 0u && model.waste_count == 24u);

    model.score = 150u;
    assert(solitaire_model_draw(&model) == SOL_ACTION_CHANGED);
    assert(model.stock_count == 24u && model.waste_count == 0u);
    assert(memcmp(model.stock, original, sizeof(original)) == 0);
    assert(model.passes == 1u);
    assert(solitaire_model_score(&model) == 50u);

    for (draw = 0u; draw != 25u; ++draw) solitaire_model_draw(&model);
    assert(solitaire_model_score(&model) == 0u);
    assert_invariants(&model);
}

static void test_draw_three_penalty(void)
{
    SolitaireModel model;
    unsigned char pass;
    unsigned char draw;

    solitaire_model_init(&model, 9u, 3u);
    model.score = 100u;
    for (pass = 1u; pass != 5u; ++pass) {
        for (draw = 0u; draw != 8u; ++draw) {
            assert(solitaire_model_draw(&model) == SOL_ACTION_CHANGED);
        }
        assert(model.stock_count == 0u && model.waste_count == 24u);
        assert(solitaire_model_draw(&model) == SOL_ACTION_CHANGED);
        assert(model.passes == pass);
        assert(solitaire_model_score(&model) ==
               ((pass <= SOL_FREE_PASSES_DRAW_THREE) ? 100u : 80u));
    }
    assert_invariants(&model);
}

static void test_empty_draw(void)
{
    SolitaireModel model;

    clear_model(&model);
    assert(solitaire_model_draw(&model) == SOL_ACTION_NONE);
    assert(solitaire_model_draw((SolitaireModel *)0) == SOL_ACTION_NONE);
}

static void test_foundation_rules(void)
{
    SolitaireModel model;

    clear_model(&model);
    model.waste[model.waste_count++] = CARD(HEARTS, 1u);
    assert(!solitaire_model_can_move(&model, SOL_PILE_WASTE, 1u, F(0)));

    model.waste[0] = CARD(HEARTS, 0u);
    assert(solitaire_model_can_move(&model, SOL_PILE_WASTE, 1u, F(2)));
    assert(solitaire_model_move(&model, SOL_PILE_WASTE, 1u, F(2)) == SOL_ACTION_CHANGED);
    assert(solitaire_model_top(&model, F(2)) == CARD(HEARTS, 0u));
    assert(solitaire_model_score(&model) == SOL_SCORE_TO_FOUNDATION);

    model.waste[model.waste_count++] = CARD(DIAMONDS, 1u);
    assert(!solitaire_model_can_move(&model, SOL_PILE_WASTE, 1u, F(2)));
    model.waste[0] = CARD(HEARTS, 2u);
    assert(!solitaire_model_can_move(&model, SOL_PILE_WASTE, 1u, F(2)));
    model.waste[0] = CARD(HEARTS, 1u);
    assert(solitaire_model_move(&model, SOL_PILE_WASTE, 1u, F(2)) == SOL_ACTION_CHANGED);
    assert(solitaire_model_pile_size(&model, F(2)) == 2u);
}

static void test_tableau_rules(void)
{
    SolitaireModel model;

    clear_model(&model);
    push_tableau(&model, 0u, CARD(SPADES, 6u));
    model.waste[model.waste_count++] = CARD(HEARTS, 5u);
    assert(solitaire_model_can_move(&model, SOL_PILE_WASTE, 1u, T(0)));
    model.waste[0] = CARD(CLUBS, 5u);
    assert(!solitaire_model_can_move(&model, SOL_PILE_WASTE, 1u, T(0)));
    model.waste[0] = CARD(HEARTS, 4u);
    assert(!solitaire_model_can_move(&model, SOL_PILE_WASTE, 1u, T(0)));

    model.waste[0] = CARD(DIAMONDS, 12u);
    assert(solitaire_model_can_move(&model, SOL_PILE_WASTE, 1u, T(1)));
    model.waste[0] = CARD(DIAMONDS, 11u);
    assert(!solitaire_model_can_move(&model, SOL_PILE_WASTE, 1u, T(1)));

    model.waste[0] = CARD(HEARTS, 5u);
    assert(solitaire_model_move(&model, SOL_PILE_WASTE, 1u, T(0)) == SOL_ACTION_CHANGED);
    assert(solitaire_model_score(&model) == SOL_SCORE_WASTE_TO_TABLEAU);
}

static void test_illegal_moves(void)
{
    SolitaireModel model;

    clear_model(&model);
    model.stock[model.stock_count++] = CARD(CLUBS, 0u);
    model.waste[model.waste_count++] = CARD(CLUBS, 1u);
    push_tableau(&model, 0u, CARD(HEARTS, 9u));
    push_tableau(&model, 0u, CARD(SPADES, 12u));
    model.tableau_hidden[0] = 1u;
    push_tableau(&model, 1u, CARD(HEARTS, 12u));

    assert(!solitaire_model_can_move(&model, SOL_PILE_STOCK, 1u, F(0)));
    assert(!solitaire_model_can_move(&model, T(0), 1u, SOL_PILE_WASTE));
    assert(!solitaire_model_can_move(&model, T(0), 1u, SOL_PILE_STOCK));
    assert(!solitaire_model_can_move(&model, T(0), 1u, T(0)));
    assert(!solitaire_model_can_move(&model, T(0), 0u, T(2)));
    assert(!solitaire_model_can_move(&model, T(0), 3u, T(2)));
    assert(!solitaire_model_can_move(&model, T(0), 2u, T(2)));
    assert(!solitaire_model_can_move(&model, SOL_PILE_WASTE, 2u, T(2)));
    assert(!solitaire_model_can_move(&model, SOL_PILE_COUNT, 1u, T(2)));
    assert(!solitaire_model_can_move(&model, T(0), 1u, SOL_PILE_COUNT));
    assert(!solitaire_model_can_move((const SolitaireModel *)0, T(0), 1u, T(2)));
    assert(solitaire_model_can_move(&model, T(0), 1u, T(2)));
    assert(solitaire_model_move(&model, T(1), 1u, T(1)) == SOL_ACTION_NONE);
}

static void test_run_move_and_flip(void)
{
    SolitaireModel model;

    clear_model(&model);
    push_tableau(&model, 0u, CARD(CLUBS, 3u));
    push_tableau(&model, 0u, CARD(HEARTS, 11u));
    push_tableau(&model, 0u, CARD(CLUBS, 10u));
    push_tableau(&model, 0u, CARD(DIAMONDS, 9u));
    model.tableau_hidden[0] = 1u;
    push_tableau(&model, 1u, CARD(SPADES, 12u));

    assert(solitaire_model_can_move(&model, T(0), 3u, T(1)));
    assert(solitaire_model_move(&model, T(0), 3u, T(1)) == SOL_ACTION_CHANGED);
    assert(solitaire_model_pile_size(&model, T(1)) == 4u);
    assert(solitaire_model_top(&model, T(1)) == CARD(DIAMONDS, 9u));
    assert(solitaire_model_pile_size(&model, T(0)) == 1u);
    assert(solitaire_model_hidden(&model, T(0)) == 0u);
    assert(solitaire_model_score(&model) == SOL_SCORE_FLIP);

    assert(solitaire_model_move(&model, T(1), 4u, T(0)) == SOL_ACTION_NONE);
}

static void test_foundation_to_tableau_penalty(void)
{
    SolitaireModel model;

    clear_model(&model);
    model.foundation_suit[0] = HEARTS;
    model.foundation_count[0] = 5u;
    push_tableau(&model, 0u, CARD(SPADES, 5u));
    model.score = 10u;
    assert(solitaire_model_move(&model, F(0), 1u, T(0)) == SOL_ACTION_CHANGED);
    assert(solitaire_model_top(&model, T(0)) == CARD(HEARTS, 4u));
    assert(solitaire_model_pile_size(&model, F(0)) == 4u);
    assert(solitaire_model_score(&model) == 0u);
}

static void test_auto_foundation(void)
{
    SolitaireModel model;

    clear_model(&model);
    model.waste[model.waste_count++] = CARD(SPADES, 0u);
    assert(solitaire_model_auto_foundation(&model, SOL_PILE_WASTE) == F(0));
    model.waste[model.waste_count++] = CARD(CLUBS, 4u);
    assert(solitaire_model_auto_foundation(&model, SOL_PILE_WASTE) == SOL_NO_PILE);
    assert(model.waste_count == 1u);

    /* Shuffling an ace between empty foundations must not farm points. */
    assert(!solitaire_model_can_move(&model, F(0), 1u, F(1)));
    assert(solitaire_model_move(&model, F(0), 1u, F(1)) == SOL_ACTION_NONE);
    assert(solitaire_model_score(&model) == SOL_SCORE_TO_FOUNDATION);
}

static void fill_foundations_except(SolitaireModel *model, unsigned char suit,
                                    unsigned char missing)
{
    unsigned char index;
    for (index = 0u; index != SOL_FOUNDATION_COUNT; ++index) {
        model->foundation_suit[index] = index;
        model->foundation_count[index] = (index == suit) ? (unsigned char)(SOL_RANKS - missing)
                                                         : SOL_RANKS;
    }
}

static void test_win(void)
{
    SolitaireModel model;

    clear_model(&model);
    fill_foundations_except(&model, SPADES, 1u);
    push_tableau(&model, 3u, CARD(SPADES, 12u));
    assert(solitaire_model_move(&model, T(3), 1u, F(3)) == SOL_ACTION_WON);
    assert(solitaire_model_is_won(&model));
    assert(solitaire_model_draw(&model) == SOL_ACTION_NONE);
    assert(!solitaire_model_can_move(&model, F(3), 1u, T(0)));
    assert_invariants(&model);
}

static void test_autocomplete(void)
{
    SolitaireModel model;
    unsigned char steps = 0u;
    unsigned char source;
    unsigned char previous_rank = 0u;
    unsigned char card;

    clear_model(&model);
    fill_foundations_except(&model, HEARTS, 4u);
    model.foundation_count[SPADES] = SOL_RANKS - 4u;
    push_tableau(&model, 0u, CARD(HEARTS, 12u));
    push_tableau(&model, 0u, CARD(SPADES, 11u));
    push_tableau(&model, 0u, CARD(HEARTS, 10u));
    push_tableau(&model, 0u, CARD(SPADES, 9u));
    push_tableau(&model, 1u, CARD(SPADES, 12u));
    push_tableau(&model, 1u, CARD(HEARTS, 11u));
    push_tableau(&model, 1u, CARD(SPADES, 10u));
    push_tableau(&model, 1u, CARD(HEARTS, 9u));
    assert_invariants(&model);

    model.stock[model.stock_count++] = CARD(CLUBS, 12u);
    model.foundation_count[CLUBS] = SOL_RANKS - 1u;
    assert(!solitaire_model_can_autocomplete(&model));
    model.stock_count = 0u;
    model.foundation_count[CLUBS] = SOL_RANKS;
    assert(solitaire_model_can_autocomplete(&model));

    while (!solitaire_model_is_won(&model)) {
        card = solitaire_model_top(&model, T(0));
        if (card == SOL_NO_CARD ||
            (solitaire_model_top(&model, T(1)) != SOL_NO_CARD &&
             SOL_RANK(solitaire_model_top(&model, T(1))) < SOL_RANK(card))) {
            card = solitaire_model_top(&model, T(1));
        }
        source = solitaire_model_autocomplete_step(&model);
        assert(source != SOL_NO_PILE);
        assert(SOL_RANK(card) >= previous_rank);
        previous_rank = SOL_RANK(card);
        assert(++steps <= 8u);
    }
    assert(steps == 8u);
    assert(solitaire_model_autocomplete_step(&model) == SOL_NO_PILE);
}

static void test_random_play_soak(void)
{
    SolitaireModel model;
    unsigned int seed;
    unsigned int step;
    unsigned int rng;
    unsigned char from;
    unsigned char to;
    unsigned char count;
    unsigned char size;
    unsigned int wins = 0u;

    for (seed = 1u; seed <= 300u; ++seed) {
        solitaire_model_init(&model, seed, (seed & 1u) ? 1u : 3u);
        rng = seed * 7919u;
        for (step = 0u; step != 3000u && !solitaire_model_is_won(&model); ++step) {
            rng = rng * 1103515245u + 12345u;
            if (solitaire_model_can_autocomplete(&model)) {
                assert(solitaire_model_autocomplete_step(&model) != SOL_NO_PILE);
            } else if ((rng >> 16) % 4u == 0u) {
                solitaire_model_draw(&model);
            } else {
                for (from = SOL_PILE_WASTE; from != SOL_PILE_COUNT; ++from) {
                    if (solitaire_model_auto_foundation(&model, from) != SOL_NO_PILE) break;
                }
                from = (unsigned char)(SOL_PILE_WASTE + (rng >> 8) % (SOL_PILE_COUNT - 1u));
                to = (unsigned char)(SOL_PILE_TABLEAU + (rng >> 20) % SOL_TABLEAU_COUNT);
                size = solitaire_model_pile_size(&model, from);
                count = (unsigned char)(size == 0u ? 0u : 1u + (rng >> 12) % size);
                if (solitaire_model_can_move(&model, from, count, to)) {
                    assert(solitaire_model_move(&model, from, count, to) != SOL_ACTION_NONE);
                } else {
                    assert(solitaire_model_move(&model, from, count, to) == SOL_ACTION_NONE);
                }
            }
            assert_invariants(&model);
        }
        if (solitaire_model_is_won(&model)) ++wins;
    }
    printf("solitaire_model: random soak won %u of 300 games\n", wins);
}

int main(void)
{
    test_deal();
    test_determinism();
    test_draw_one_and_recycle();
    test_draw_three_penalty();
    test_empty_draw();
    test_foundation_rules();
    test_tableau_rules();
    test_illegal_moves();
    test_run_move_and_flip();
    test_foundation_to_tableau_penalty();
    test_auto_foundation();
    test_win();
    test_autocomplete();
    test_random_play_soak();
    puts("solitaire_model: all tests passed");
    return 0;
}
