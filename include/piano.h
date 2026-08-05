#ifndef GBW_PIANO_H
#define GBW_PIANO_H

#include "app.h"
#include "input.h"

void piano_enter(void);
AppState piano_update(const InputState *input);

#endif
