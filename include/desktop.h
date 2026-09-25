#ifndef GBW_DESKTOP_H
#define GBW_DESKTOP_H

#include <gb/gb.h>
#include "app.h"
#include "input.h"

void desktop_enter(void) BANKED;
AppState desktop_update(const InputState *input) BANKED;

/* Redraw the Program Manager, inactive, behind an application window.
 * Call between ui_scene_begin() and ui_scene_end(). */
void desktop_draw_backdrop(void) BANKED;

#endif
