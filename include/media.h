#ifndef GBW_MEDIA_H
#define GBW_MEDIA_H

#include <gb/gb.h>
#include "app.h"
#include "input.h"

void media_enter(void) BANKED;
AppState media_update(const InputState *input) BANKED;

#endif
