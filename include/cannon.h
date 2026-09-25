#ifndef GBW_CANNON_H
#define GBW_CANNON_H

#include <gb/gb.h>
#include "app.h"
#include "input.h"

void cannon_enter(void) BANKED;
AppState cannon_update(const InputState *input) BANKED;

#endif
