#ifndef UI_H
#define UI_H

#include <gb/gb.h> // For uint8_t
#include "app_states.h" // Include APP_STATE definition

// --- GBSWINDOWS Layout Constants ---
// Screen dimensions (tiles)
#define SCREEN_TILE_WIDTH  20
#define SCREEN_TILE_HEIGHT 18

// Main Window ("GBSWINDOWS")
#define MW_X 1
#define MW_Y 1
#define MW_W 18
#define MW_H 16
#define MW_TITLE "GBSWINDOWS"

// Accessories Sub-Window
#define SW_ACC_X (MW_X + 1)
#define SW_ACC_Y (MW_Y + 2)
#define SW_ACC_W 8
#define SW_ACC_H 7
#define SW_ACC_TITLE "Accessories"

// Games Sub-Window
#define SW_GAMES_X (SW_ACC_X + SW_ACC_W + 1)
#define SW_GAMES_Y (MW_Y + 2)
#define SW_GAMES_W 8
#define SW_GAMES_H 7
#define SW_GAMES_TITLE "Games"

// Icon properties (common)
#define ICON_TILE_WIDTH 2 // Icons are 2 tiles wide
#define ICON_TILE_HEIGHT 1 // Icons are 1 tile high (can be 2x2 if data changes)
#define ICON_TEXT_OFFSET_Y 2 // Text label Y offset below icon tiles

// Paint Icon (in Accessories)
#define ICON_PAINT_REL_X 1 // Relative X within sub-window content area
#define ICON_PAINT_REL_Y 1 // Relative Y within sub-window content area
#define ICON_PAINT_ABS_TILE_X (SW_ACC_X + 1 + ICON_PAINT_REL_X) // +1 for sub-window border
#define ICON_PAINT_ABS_TILE_Y (SW_ACC_Y + 2 + ICON_PAINT_REL_Y) // +2 for sub-window border & title
#define ICON_PAINT_TEXT_X ICON_PAINT_ABS_TILE_X
#define ICON_PAINT_TEXT_Y (ICON_PAINT_ABS_TILE_Y + ICON_TEXT_OFFSET_Y)

// Minesweeper Icon (in Games)
#define ICON_MINESWEEPER_REL_X 1
#define ICON_MINESWEEPER_REL_Y 1
#define ICON_MINESWEEPER_ABS_TILE_X (SW_GAMES_X + 1 + ICON_MINESWEEPER_REL_X)
#define ICON_MINESWEEPER_ABS_TILE_Y (SW_GAMES_Y + 2 + ICON_MINESWEEPER_REL_Y)
#define ICON_MINESWEEPER_TEXT_X ICON_MINESWEEPER_ABS_TILE_X
#define ICON_MINESWEEPER_TEXT_Y (ICON_MINESWEEPER_ABS_TILE_Y + ICON_TEXT_OFFSET_Y)

// Exit Icon (in Games)
#define ICON_EXIT_REL_X (ICON_MINESWEEPER_REL_X + ICON_TILE_WIDTH + 2) // Place it next to Minesweeper
#define ICON_EXIT_REL_Y 1
#define ICON_EXIT_ABS_TILE_X (SW_GAMES_X + 1 + ICON_EXIT_REL_X)
#define ICON_EXIT_ABS_TILE_Y (SW_GAMES_Y + 2 + ICON_EXIT_REL_Y)
#define ICON_EXIT_TEXT_X ICON_EXIT_ABS_TILE_X
#define ICON_EXIT_TEXT_Y (ICON_EXIT_ABS_TILE_Y + ICON_TEXT_OFFSET_Y)

#define UI_CURSOR_SPRITE_ID 0 // Define the sprite ID for the UI cursor

#define MAX_ICONS 3

// New GBSWINDOWS Style UI Layout Constants
#define GBS_SCREEN_TILE_WIDTH 20
#define GBS_SCREEN_TILE_HEIGHT 18

// Program Manager Window (main window)
#define GBS_MAIN_WIN_X 1         // Tile X
#define GBS_MAIN_WIN_Y 1         // Tile Y
#define GBS_MAIN_WIN_W 18        // Tile width (20 - 1 - 1)
#define GBS_MAIN_WIN_H 15        // Tile height (18 - 1 - 2, leaving space for potential status bar or just margin)

#define GBS_TITLE_BAR_HEIGHT 1   // In tiles

// Icon properties within the main window's content area
#define GBS_ICON_AREA_X (GBS_MAIN_WIN_X + 1) // Inner X relative to screen
#define GBS_ICON_AREA_Y (GBS_MAIN_WIN_Y + GBS_TITLE_BAR_HEIGHT + 1) // Inner Y
#define GBS_ICON_AREA_W (GBS_MAIN_WIN_W - 2)
#define GBS_ICON_AREA_H (GBS_MAIN_WIN_H - GBS_TITLE_BAR_HEIGHT - 2)

#define GBS_ICON_TILE_WIDTH 2  // Each icon graphic is 2 tiles wide (16px)
#define GBS_ICON_TILE_HEIGHT 1 // Each icon graphic is 1 tile high (8px) - this might be 2 (16px) for better detail
                               // Let's assume 2x2 tiles (16x16 px) for icons for now, meaning 4 tiles per icon graphic.
                               // The TILE_IDX_GBS_ICON_..._START assumes 2 tiles (16x8). This needs to be reconciled.
                               // For now, sticking to 16x8px (2 tiles wide, 1 tile high) as per current tile defs.

#define GBS_ICON_LABEL_MAX_LEN 12 // Max characters for an icon label
#define GBS_ICON_SPACING_X 4      // Horizontal space between icons (center to center in tiles)
#define GBS_ICON_SPACING_Y 3      // Vertical space between icons (center to center in tiles)

#define GBS_ICON_START_X (GBS_ICON_AREA_X + 2) // Starting X for the first icon (tile units)
#define GBS_ICON_START_Y (GBS_ICON_AREA_Y + 1) // Starting Y for the first icon (tile units)

// Cursor properties for GBS style
#define GBS_CURSOR_SPRITE_IDX 0 // Sprite index for the GBS cursor (if different from old one)

// Main function to run the home screen (Program Manager)
UINT8 run_home_screen(void);

// Function to set attributes for a rectangle of background tiles
void fill_bkg_rect_attributes(UINT8 x, UINT8 y, UINT8 w, UINT8 h, UINT8 attributes);

// Function to set attributes for a single background tile
void set_bkg_attributes_xy(UINT8 x, UINT8 y, UINT8 attributes);

// Helper to draw text on the background
void draw_text_bkg_clipped(UBYTE x, UBYTE y, const char* text, UBYTE palette_idx, UBYTE max_width, UBYTE bg_fill_tile, UBYTE bg_palette_idx);

// Function Prototypes
void ui_init(void);
void ui_home_draw_desktop_and_windows(void);
UBYTE* get_selected_icon_idx_ptr(void);
void ui_home_handle_input(UINT8 joypad_data, APP_STATE* current_app_state, UBYTE* selected_icon_idx);
void draw_window_frame(UBYTE x, UBYTE y, UBYTE w, UBYTE h, UBYTE border_palette_idx, UBYTE content_palette_idx, UBYTE content_tile_idx);
void draw_window_title_bar(UBYTE x, UBYTE y, UBYTE w, const char* title, UBYTE title_palette_idx, UBYTE frame_palette_idx, UBYTE is_active);

// New GBS style drawing functions (prototypes)
void gbs_draw_window(UBYTE x, UBYTE y, UBYTE w, UBYTE h, const char* title, UBYTE is_active);
void gbs_draw_icon(UBYTE screen_x, UBYTE screen_y, UBYTE icon_tile_start_idx, const char* label, UBYTE icon_gfx_pal, UBYTE icon_text_pal, UBYTE is_selected);

#endif