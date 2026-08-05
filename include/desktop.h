#ifndef GBW_DESKTOP_H
#define GBW_DESKTOP_H

#include "app.h"
#include "input.h"

void desktop_enter(void);
AppState desktop_update(const InputState *input);

#endif
