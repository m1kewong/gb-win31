#ifndef GBW_BOOT_H
#define GBW_BOOT_H

#include <gb/gb.h>
#include "app.h"
#include "input.h"

void boot_enter(void) BANKED;
AppState boot_update(const InputState *input) BANKED;
void dos_enter(void) BANKED;
AppState dos_update(const InputState *input) BANKED;

#endif
