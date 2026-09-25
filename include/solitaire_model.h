#ifndef SOLITAIRE_MODEL_H
#define SOLITAIRE_MODEL_H

/* Klondike rules with Windows-style standard scoring. Plain C so the same
 * file builds for the Game Boy and for host unit tests. */

#define SOL_CARD_COUNT 52u
#define SOL_RANKS 13u
#define SOL_TABLEAU_COUNT 7u
#define SOL_FOUNDATION_COUNT 4u
#define SOL_TABLEAU_MAX 19u
#define SOL_STOCK_MAX 24u

/* Pile identifiers. */
#define SOL_PILE_STOCK 0u
#define SOL_PILE_WASTE 1u
#define SOL_PILE_FOUNDATION 2u
#define SOL_PILE_TABLEAU (SOL_PILE_FOUNDATION + SOL_FOUNDATION_COUNT)
#define SOL_PILE_COUNT (SOL_PILE_TABLEAU + SOL_TABLEAU_COUNT)
#define SOL_NO_PILE 0xffu
#define SOL_NO_CARD 0xffu

/* Cards are suit * 13 + rank; rank 0 is the ace and 12 the king.
 * Suits: 0 clubs, 1 diamonds, 2 hearts, 3 spades. */
#define SOL_RANK(card) ((unsigned char)((card) % SOL_RANKS))
#define SOL_SUIT(card) ((unsigned char)((card) / SOL_RANKS))
#define SOL_IS_RED(card) (SOL_SUIT(card) == 1u || SOL_SUIT(card) == 2u)

#define SOL_ACTION_NONE 0u
#define SOL_ACTION_CHANGED 1u
#define SOL_ACTION_WON 2u

#define SOL_PLAYING 0u
#define SOL_WON 1u

#define SOL_SCORE_WASTE_TO_TABLEAU 5u
#define SOL_SCORE_TO_FOUNDATION 10u
#define SOL_SCORE_FLIP 5u
#define SOL_PENALTY_FOUNDATION_TO_TABLEAU 15u
#define SOL_PENALTY_RECYCLE_DRAW_ONE 100u
#define SOL_PENALTY_RECYCLE_DRAW_THREE 20u
#define SOL_FREE_PASSES_DRAW_THREE 3u

typedef struct SolitaireModel {
    unsigned char stock[SOL_STOCK_MAX];
    unsigned char waste[SOL_STOCK_MAX];
    unsigned char tableau[SOL_TABLEAU_COUNT][SOL_TABLEAU_MAX];
    unsigned char tableau_count[SOL_TABLEAU_COUNT];
    unsigned char tableau_hidden[SOL_TABLEAU_COUNT];
    unsigned char foundation_count[SOL_FOUNDATION_COUNT];
    unsigned char foundation_suit[SOL_FOUNDATION_COUNT];
    unsigned char stock_count;
    unsigned char waste_count;
    unsigned char draw_count;
    unsigned char passes;
    unsigned char status;
    unsigned int rng_state;
    unsigned int score;
} SolitaireModel;

void solitaire_model_init(SolitaireModel *model, unsigned int seed,
                          unsigned char draw_count);

/* Turn cards from the stock to the waste, or recycle the waste when the
 * stock is empty. */
unsigned char solitaire_model_draw(SolitaireModel *model);

unsigned char solitaire_model_pile_size(const SolitaireModel *model,
                                        unsigned char pile);
/* Card `index` counted from the bottom of the pile, or SOL_NO_CARD. */
unsigned char solitaire_model_card_at(const SolitaireModel *model,
                                      unsigned char pile, unsigned char index);
unsigned char solitaire_model_top(const SolitaireModel *model, unsigned char pile);
/* Face-down cards at the bottom of a tableau pile (0 for other piles). */
unsigned char solitaire_model_hidden(const SolitaireModel *model,
                                     unsigned char pile);

/* Move the top `count` cards of `from` onto `to`. Only tableau piles may
 * move more than one card. */
unsigned char solitaire_model_can_move(const SolitaireModel *model,
                                       unsigned char from, unsigned char count,
                                       unsigned char to);
unsigned char solitaire_model_move(SolitaireModel *model, unsigned char from,
                                   unsigned char count, unsigned char to);

/* Send the top card of `from` to a foundation that accepts it; returns the
 * foundation pile or SOL_NO_PILE. */
unsigned char solitaire_model_auto_foundation(SolitaireModel *model,
                                              unsigned char from);

/* True once every card is face up and stock and waste are empty. */
unsigned char solitaire_model_can_autocomplete(const SolitaireModel *model);
/* Play the lowest card that fits a foundation; returns the source pile or
 * SOL_NO_PILE when nothing moved. */
unsigned char solitaire_model_autocomplete_step(SolitaireModel *model);

unsigned char solitaire_model_is_won(const SolitaireModel *model);
unsigned int solitaire_model_score(const SolitaireModel *model);

#endif
