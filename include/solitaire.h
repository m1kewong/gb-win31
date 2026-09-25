#ifndef GBW_SOLITAIRE_H
#define GBW_SOLITAIRE_H

#include <gb/gb.h>
#include "app.h"
#include "input.h"
#include "solitaire_model.h"

void solitaire_enter(void) BANKED;
AppState solitaire_update(const InputState *input) BANKED;

#endif
