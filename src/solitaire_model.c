#include "solitaire_model.h"

static unsigned int sol_next_random(SolitaireModel *model)
{
    model->rng_state = (unsigned int)((model->rng_state * 25173u + 13849u) & 65535u);
    return model->rng_state;
}

static unsigned char sol_is_foundation(unsigned char pile)
{
    return (unsigned char)(pile >= SOL_PILE_FOUNDATION && pile < SOL_PILE_TABLEAU);
}

static unsigned char sol_is_tableau(unsigned char pile)
{
    return (unsigned char)(pile >= SOL_PILE_TABLEAU && pile < SOL_PILE_COUNT);
}

static void sol_add_score(SolitaireModel *model, unsigned int points)
{
    model->score += points;
}

static void sol_subtract_score(SolitaireModel *model, unsigned int points)
{
    model->score = (model->score > points) ? (unsigned int)(model->score - points) : 0u;
}

void solitaire_model_init(SolitaireModel *model, unsigned int seed,
                          unsigned char draw_count)
{
    unsigned char deck[SOL_CARD_COUNT];
    unsigned char index;
    unsigned char swap;
    unsigned char temporary;
    unsigned char pile;
    unsigned char row;
    unsigned char next;

    if (model == (SolitaireModel *)0) return;

    model->rng_state = (unsigned int)(seed & 65535u);
    for (index = 0u; index != SOL_CARD_COUNT; ++index) deck[index] = index;
    for (index = (unsigned char)(SOL_CARD_COUNT - 1u); index != 0u; --index) {
        swap = (unsigned char)((sol_next_random(model) >> 4u) % (unsigned int)(index + 1u));
        temporary = deck[index];
        deck[index] = deck[swap];
        deck[swap] = temporary;
    }

    next = 0u;
    for (pile = 0u; pile != SOL_TABLEAU_COUNT; ++pile) {
        for (row = 0u; row != SOL_TABLEAU_MAX; ++row) model->tableau[pile][row] = SOL_NO_CARD;
        for (row = 0u; row <= pile; ++row) model->tableau[pile][row] = deck[next++];
        model->tableau_count[pile] = (unsigned char)(pile + 1u);
        model->tableau_hidden[pile] = pile;
    }

    model->stock_count = 0u;
    while (next != SOL_CARD_COUNT) model->stock[model->stock_count++] = deck[next++];
    model->waste_count = 0u;
    for (index = 0u; index != SOL_STOCK_MAX; ++index) model->waste[index] = SOL_NO_CARD;

    for (pile = 0u; pile != SOL_FOUNDATION_COUNT; ++pile) {
        model->foundation_count[pile] = 0u;
        model->foundation_suit[pile] = 0u;
    }

    model->draw_count = (draw_count == 3u) ? 3u : 1u;
    model->passes = 0u;
    model->status = SOL_PLAYING;
    model->score = 0u;
}

unsigned char solitaire_model_draw(SolitaireModel *model)
{
    unsigned char turned;

    if (model == (SolitaireModel *)0 || model->status != SOL_PLAYING) {
        return SOL_ACTION_NONE;
    }

    if (model->stock_count == 0u) {
        if (model->waste_count == 0u) return SOL_ACTION_NONE;
        while (model->waste_count != 0u) {
            model->stock[model->stock_count++] = model->waste[--model->waste_count];
        }
        ++model->passes;
        if (model->draw_count == 1u) {
            sol_subtract_score(model, SOL_PENALTY_RECYCLE_DRAW_ONE);
        } else if (model->passes > SOL_FREE_PASSES_DRAW_THREE) {
            sol_subtract_score(model, SOL_PENALTY_RECYCLE_DRAW_THREE);
        }
        return SOL_ACTION_CHANGED;
    }

    for (turned = 0u; turned != model->draw_count && model->stock_count != 0u; ++turned) {
        model->waste[model->waste_count++] = model->stock[--model->stock_count];
    }
    return SOL_ACTION_CHANGED;
}

unsigned char solitaire_model_pile_size(const SolitaireModel *model, unsigned char pile)
{
    if (model == (const SolitaireModel *)0) return 0u;
    if (pile == SOL_PILE_STOCK) return model->stock_count;
    if (pile == SOL_PILE_WASTE) return model->waste_count;
    if (sol_is_foundation(pile)) return model->foundation_count[pile - SOL_PILE_FOUNDATION];
    if (sol_is_tableau(pile)) return model->tableau_count[pile - SOL_PILE_TABLEAU];
    return 0u;
}

unsigned char solitaire_model_card_at(const SolitaireModel *model,
                                      unsigned char pile, unsigned char index)
{
    unsigned char foundation;

    if (index >= solitaire_model_pile_size(model, pile)) return SOL_NO_CARD;
    if (pile == SOL_PILE_STOCK) return model->stock[index];
    if (pile == SOL_PILE_WASTE) return model->waste[index];
    if (sol_is_foundation(pile)) {
        foundation = (unsigned char)(pile - SOL_PILE_FOUNDATION);
        return (unsigned char)(model->foundation_suit[foundation] * SOL_RANKS + index);
    }
    return model->tableau[pile - SOL_PILE_TABLEAU][index];
}

unsigned char solitaire_model_top(const SolitaireModel *model, unsigned char pile)
{
    unsigned char size = solitaire_model_pile_size(model, pile);
    if (size == 0u) return SOL_NO_CARD;
    return solitaire_model_card_at(model, pile, (unsigned char)(size - 1u));
}

unsigned char solitaire_model_hidden(const SolitaireModel *model, unsigned char pile)
{
    if (model == (const SolitaireModel *)0 || !sol_is_tableau(pile)) return 0u;
    return model->tableau_hidden[pile - SOL_PILE_TABLEAU];
}

unsigned char solitaire_model_can_move(const SolitaireModel *model,
                                       unsigned char from, unsigned char count,
                                       unsigned char to)
{
    unsigned char size;
    unsigned char base;
    unsigned char target;
    unsigned char foundation;

    if (model == (const SolitaireModel *)0 || model->status != SOL_PLAYING ||
        from >= SOL_PILE_COUNT || to >= SOL_PILE_COUNT || from == to ||
        from == SOL_PILE_STOCK || count == 0u ||
        (sol_is_foundation(from) && sol_is_foundation(to))) return 0u;

    size = solitaire_model_pile_size(model, from);
    if (count > size || count > SOL_RANKS) return 0u;
    if (count > 1u && !sol_is_tableau(from)) return 0u;
    if (sol_is_tableau(from) &&
        (unsigned char)(size - count) < model->tableau_hidden[from - SOL_PILE_TABLEAU]) {
        return 0u;
    }
    base = solitaire_model_card_at(model, from, (unsigned char)(size - count));

    if (sol_is_foundation(to)) {
        if (count != 1u) return 0u;
        foundation = (unsigned char)(to - SOL_PILE_FOUNDATION);
        if (model->foundation_count[foundation] == 0u) return (unsigned char)(SOL_RANK(base) == 0u);
        return (unsigned char)(SOL_SUIT(base) == model->foundation_suit[foundation] &&
                               SOL_RANK(base) == model->foundation_count[foundation]);
    }

    if (sol_is_tableau(to)) {
        if (model->tableau_count[to - SOL_PILE_TABLEAU] + count > SOL_TABLEAU_MAX) return 0u;
        target = solitaire_model_top(model, to);
        if (target == SOL_NO_CARD) return (unsigned char)(SOL_RANK(base) == SOL_RANKS - 1u);
        return (unsigned char)(SOL_RANK(target) == (unsigned char)(SOL_RANK(base) + 1u) &&
                               SOL_IS_RED(target) != SOL_IS_RED(base));
    }
    return 0u;
}

static unsigned char sol_take(SolitaireModel *model, unsigned char pile)
{
    unsigned char foundation;
    unsigned char column;

    if (pile == SOL_PILE_WASTE) return model->waste[--model->waste_count];
    if (sol_is_foundation(pile)) {
        foundation = (unsigned char)(pile - SOL_PILE_FOUNDATION);
        --model->foundation_count[foundation];
        return (unsigned char)(model->foundation_suit[foundation] * SOL_RANKS +
                               model->foundation_count[foundation]);
    }
    column = (unsigned char)(pile - SOL_PILE_TABLEAU);
    --model->tableau_count[column];
    return model->tableau[column][model->tableau_count[column]];
}

static void sol_put(SolitaireModel *model, unsigned char pile, unsigned char card)
{
    unsigned char foundation;
    unsigned char column;

    if (sol_is_foundation(pile)) {
        foundation = (unsigned char)(pile - SOL_PILE_FOUNDATION);
        model->foundation_suit[foundation] = SOL_SUIT(card);
        ++model->foundation_count[foundation];
        return;
    }
    column = (unsigned char)(pile - SOL_PILE_TABLEAU);
    model->tableau[column][model->tableau_count[column]++] = card;
}

unsigned char solitaire_model_move(SolitaireModel *model, unsigned char from,
                                   unsigned char count, unsigned char to)
{
    unsigned char moving[SOL_RANKS];
    unsigned char index;
    unsigned char column;
    unsigned int total;

    if (!solitaire_model_can_move(model, from, count, to)) return SOL_ACTION_NONE;

    for (index = count; index != 0u; --index) moving[index - 1u] = sol_take(model, from);
    for (index = 0u; index != count; ++index) sol_put(model, to, moving[index]);

    if (sol_is_foundation(to)) {
        sol_add_score(model, SOL_SCORE_TO_FOUNDATION);
    } else if (from == SOL_PILE_WASTE) {
        sol_add_score(model, SOL_SCORE_WASTE_TO_TABLEAU);
    } else if (sol_is_foundation(from)) {
        sol_subtract_score(model, SOL_PENALTY_FOUNDATION_TO_TABLEAU);
    }

    if (sol_is_tableau(from)) {
        column = (unsigned char)(from - SOL_PILE_TABLEAU);
        if (model->tableau_count[column] != 0u &&
            model->tableau_hidden[column] == model->tableau_count[column]) {
            --model->tableau_hidden[column];
            sol_add_score(model, SOL_SCORE_FLIP);
        }
    }

    total = 0u;
    for (index = 0u; index != SOL_FOUNDATION_COUNT; ++index) {
        total += model->foundation_count[index];
    }
    if (total == SOL_CARD_COUNT) {
        model->status = SOL_WON;
        return SOL_ACTION_WON;
    }
    return SOL_ACTION_CHANGED;
}

static unsigned char sol_foundation_for(const SolitaireModel *model, unsigned char from)
{
    unsigned char pile;

    for (pile = SOL_PILE_FOUNDATION; pile != SOL_PILE_TABLEAU; ++pile) {
        if (solitaire_model_can_move(model, from, 1u, pile)) return pile;
    }
    return SOL_NO_PILE;
}

unsigned char solitaire_model_auto_foundation(SolitaireModel *model, unsigned char from)
{
    unsigned char pile = sol_foundation_for(model, from);

    if (pile != SOL_NO_PILE) solitaire_model_move(model, from, 1u, pile);
    return pile;
}

unsigned char solitaire_model_can_autocomplete(const SolitaireModel *model)
{
    unsigned char column;

    if (model == (const SolitaireModel *)0 || model->status != SOL_PLAYING ||
        model->stock_count != 0u || model->waste_count != 0u) return 0u;
    for (column = 0u; column != SOL_TABLEAU_COUNT; ++column) {
        if (model->tableau_hidden[column] != 0u) return 0u;
    }
    return 1u;
}

unsigned char solitaire_model_autocomplete_step(SolitaireModel *model)
{
    unsigned char pile;
    unsigned char best = SOL_NO_PILE;
    unsigned char best_rank = SOL_RANKS;
    unsigned char card;

    if (!solitaire_model_can_autocomplete(model)) return SOL_NO_PILE;
    for (pile = SOL_PILE_TABLEAU; pile != SOL_PILE_COUNT; ++pile) {
        card = solitaire_model_top(model, pile);
        if (card == SOL_NO_CARD || SOL_RANK(card) >= best_rank) continue;
        if (sol_foundation_for(model, pile) != SOL_NO_PILE) {
            best = pile;
            best_rank = SOL_RANK(card);
        }
    }
    if (best != SOL_NO_PILE) solitaire_model_auto_foundation(model, best);
    return best;
}

unsigned char solitaire_model_is_won(const SolitaireModel *model)
{
    return (unsigned char)(model != (const SolitaireModel *)0 && model->status == SOL_WON);
}

unsigned int solitaire_model_score(const SolitaireModel *model)
{
    return (model == (const SolitaireModel *)0) ? 0u : model->score;
}
