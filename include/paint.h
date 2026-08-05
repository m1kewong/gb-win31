#ifndef GBW_PAINT_H
#define GBW_PAINT_H

#include "app.h"
#include "input.h"

#define PAINT_CANVAS_WIDTH_TILES 14u
#define PAINT_CANVAS_HEIGHT_TILES 10u

void paint_enter(void);
AppState paint_update(const InputState *input);

#endif
