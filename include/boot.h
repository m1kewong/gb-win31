#ifndef GBW_BOOT_H
#define GBW_BOOT_H

#include "app.h"
#include "input.h"

void boot_enter(void);
AppState boot_update(const InputState *input);
void dos_enter(void);
AppState dos_update(const InputState *input);

#endif
