#ifndef HW_CTRL_H
#define HW_CTRL_H

#include "selector.h"
#include "deck.h"

struct hw_state {
    struct selector *sel;
    struct deck *decks;
    int num_decks;
    int active_deck;
};

int hw_ctrl_init(struct hw_state *hw);

#endif