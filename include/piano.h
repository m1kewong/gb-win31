#ifndef GBW_PIANO_H
#define GBW_PIANO_H

#include <gb/gb.h>
#include "app.h"
#include "input.h"

void piano_enter(void) BANKED;
AppState piano_update(const InputState *input) BANKED;

#endif
