#ifndef PAINT_H
#define PAINT_H

#include <gb/gb.h>    // Added missing include
#include <gb/cgb.h>   // Added missing include
#include "tiles.h"  // For TILE_IDX_PAINT_...

// Paint application states
#define PAINT_STATE_DRAWING 0
#define PAINT_STATE_COLOR_SELECT 1 // Example if you add color selection mode
#define PAINT_STATE_TOOL_SELECT 2  // Example if you add tool selection mode
#define PAINT_STATE_EXIT 3

// Canvas definition (in tiles)
// Screen is 20x18 tiles.
#define PAINT_CANVAS_X          0  // Starting X tile on screen (0-19)
#define PAINT_CANVAS_Y          2  // Starting Y tile on screen (0-17)
#define PAINT_CANVAS_WIDTH      20 // Width in tiles (20 - 2*PAINT_CANVAS_X)
#define PAINT_CANVAS_HEIGHT     14 // Height in tiles (18 - PAINT_CANVAS_Y - UI_AREA_HEIGHT)

// UI area definition (in tiles)
#define PAINT_UI_X              0  // Starting X tile on screen (0-19)
#define PAINT_UI_Y              0  // Starting Y tile on screen (0-17)
#define PAINT_UI_WIDTH          20 // Width in tiles
#define PAINT_UI_HEIGHT         2  // Height in tiles

// Cursor pixel coordinates and corresponding screen tile
typedef struct {
    INT16 x_px;     // Pixel X coordinate (relative to screen top-left 0,0)
    INT16 y_px;     // Pixel Y coordinate (relative to screen top-left 0,0)
    UINT8 tile_x;   // Current tile X the cursor is over (screen coordinates)
    UINT8 tile_y;   // Current tile Y the cursor is over (screen coordinates)
} PaintCursorPosition;

// Drawing color tile indices are now defined in tiles.h (e.g., TILE_IDX_PAINT_DRAW_COLOR_1)
// and used in conjunction with PAINT_TILE_VRAM_OFFSET in paint.c.
// The old PAINT_COLOR_* macros are removed.

// Function prototype
UINT8 start_paint(void);

#endif // PAINT_H