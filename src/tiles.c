#include <gb/gb.h>
#include <gb/cgb.h> // For GBDK's RGB macro
#include <string.h> // For memcpy
#include "../include/tiles.h" // General tile defs, palette indices
#include "../include/minesweeper.h" // For TILE_COUNT_MS_... and MINESWEEPER_CURSOR_SPRITE_TILE_COUNT
#include "../include/paint.h"      // For TILE_COUNT_PAINT_... and PAINT_CURSOR_SPRITE_TILE_COUNT (if needed here)
#include "../include/ui.h"        // For UI_CURSOR_SPRITE_TILE_COUNT (if needed here)

// New GBSWINDOWS Style Palettes (based on the latest screenshot)
const UWORD gbs_palette_desktop[] = {
    RGB(20, 28, 30), RGB(18, 26, 28), RGB(15, 22, 25), RGB(31, 31, 31) // Teal-ish background
};
const UWORD gbs_palette_window_frame[] = {
    RGB(24, 24, 24), RGB(31, 31, 31), RGB(12, 12, 12), RGB(0, 0, 0)    // Greys for frame, white, black
};
const UWORD gbs_palette_title_active[] = {
    RGB(5, 10, 25), RGB(31, 31, 31), RGB(10, 15, 28), RGB(7, 12, 26)   // Dark blue, white text
};
const UWORD gbs_palette_title_inactive[] = {
    RGB(18, 20, 22), RGB(8, 8, 8), RGB(22, 22, 22), RGB(15, 15, 15)    // Greyed out blue/grey, dark text
};
const UWORD gbs_palette_window_content[] = { // Renamed from gbs_palette_content_area
    RGB(31, 31, 31), RGB(0, 0, 0), RGB(28, 28, 28), RGB(20, 20, 20)    // White background, black text, greys
};
const UWORD gbs_palette_icons_gfx[] = { // For program icon graphics
    RGB(0,0,0), RGB(10,10,10), RGB(20,20,20), RGB(31,31,31) // Placeholder: Black, Greys, White
};
const UWORD gbs_palette_icon_text[] = {
    RGB(20, 28, 30), RGB(0, 0, 0), RGB(8, 8, 8), RGB(31, 31, 31) // Desktop bg (transparent), black text, dk grey shadow, white highlight
};
const UWORD gbs_palette_scrollbar[] = {
    RGB(26, 26, 26), RGB(20, 20, 20), RGB(0, 0, 0), RGB(22, 22, 22) // Light greys, black for arrows/thumb details
};

// Palette for selected icon text (e.g., inverted colors or highlight)
const UWORD gbs_palette_icon_text_selected[] = {
    RGB(0, 0, 15), RGB(31, 31, 31), RGB(0,0,0), RGB(20,20,20) // Example: Dark blue BG, white text
};

// Tile data
const UBYTE gbs_desktop_fill_tile_data[] = { // Solid Color 0 of gbs_palette_desktop
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

const UBYTE gbs_content_fill_tile_data[] = { // Solid Color 0 of gbs_palette_window_content (typically white)
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

// Consolidated window frame tiles (8 tiles)
const UBYTE gbs_frame_tiles_data[] = {
    // TL (Top-Left)
    0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // Placeholder
    // T (Top)
    0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // Placeholder
    // TR (Top-Right)
    0x00,0x00,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // Placeholder
    // L (Left)
    0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00, // Placeholder
    // R (Right)
    0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF, // Placeholder
    // BL (Bottom-Left)
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00, // Placeholder
    // B (Bottom)
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00, // Placeholder
    // BR (Bottom-Right)
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x00,0x00,0x00,0x00  // Placeholder
};

// Consolidated active title bar tiles (3 tiles: L, M, R)
const UBYTE gbs_title_bar_tiles_data[] = {
    // L (Left end)
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, // Placeholder (Solid Color 0)
    // M (Middle, repeatable)
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, // Placeholder (Solid Color 0)
    // R (Right end)
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF  // Placeholder (Solid Color 0)
};

// Consolidated inactive title bar tiles (3 tiles: L, M, R)
const UBYTE gbs_title_bar_inactive_tiles_data[] = {
    // L (Left end, inactive)
    0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA, // Placeholder (Dithered Color 0/1)
    // M (Middle, inactive)
    0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA, // Placeholder (Dithered Color 0/1)
    // R (Right end, inactive)
    0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA  // Placeholder (Dithered Color 0/1)
};

// Consolidated window decoration button tiles (3 tiles: min, max, close)
const UBYTE gbs_decoration_tiles_data[] = {
    // Min button
    0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F, // Placeholder
    // Max button
    0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0, // Placeholder
    // Close button
    0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00  // Placeholder
};

// Consolidated scrollbar tiles (4 tiles: Arrow U, Arrow D, Thumb M, Track V)
const UBYTE gbs_scrollbar_tiles_data[] = {
    // Arrow Up
    0x18,0x3C,0x7E,0xFF,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18, // Placeholder Up arrow
    // Arrow Down
    0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0xFF,0x7E,0x3C,0x18,0x00,0x00,0x00,0x00, // Placeholder Down arrow
    // Thumb Middle
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, // Placeholder Solid block
    // Track Vertical
    0x55,0x55,0xAA,0xAA,0x55,0x55,0xAA,0xAA,0x55,0x55,0xAA,0xAA,0x55,0x55,0xAA,0xAA  // Placeholder Dithered track
};

UBYTE blank_tile_data[16] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

void load_all_palettes(void) {
    // Set new GBSWINDOWS style palettes (ensure these match PAL_IDX defines in tiles.h)
    set_bkg_palette(PAL_IDX_GBS_DESKTOP, 1, gbs_palette_desktop);             // Palette 0
    set_bkg_palette(PAL_IDX_GBS_WINDOW_FRAME, 1, gbs_palette_window_frame);   // Palette 1
    set_bkg_palette(PAL_IDX_GBS_TITLE_ACTIVE, 1, gbs_palette_title_active);     // Palette 2
    set_bkg_palette(PAL_IDX_GBS_WINDOW_CONTENT, 1, gbs_palette_window_content); // Palette 3
    set_bkg_palette(PAL_IDX_GBS_ICONS_GFX, 1, gbs_palette_icons_gfx);         // Palette 4
    set_bkg_palette(PAL_IDX_GBS_ICON_TEXT, 1, gbs_palette_icon_text);         // Palette 5
    set_bkg_palette(PAL_IDX_GBS_TITLE_INACTIVE, 1, gbs_palette_title_inactive); // Palette 6
    set_bkg_palette(PAL_IDX_GBS_SCROLLBAR, 1, gbs_palette_scrollbar);         // Palette 7
    set_bkg_palette(PAL_IDX_GBS_ICON_TEXT_SELECTED, 1, gbs_palette_icon_text_selected); // Palette 8

    // Load sprite palettes if needed, e.g. for cursor
    set_sprite_palette((UINT8)PAL_IDX_SPRITE_UI_CURSOR, 1, ui_cursor_sprite_palette_win31);
    set_sprite_palette((UINT8)PAL_IDX_SPRITE_MS_CURSOR, 1, minesweeper_cursor_sprite_palette);
    set_sprite_palette((UINT8)PAL_IDX_SPRITE_PAINT_CURSOR, 1, paint_cursor_sprite_palette);
}

void load_all_tiles(void) {
    // Load blank tile at a known index (e.g., TILE_IDX_BLANK)
    set_bkg_data(TILE_IDX_BLANK, 1, blank_tile_data);

    // Load new GBSWINDOWS style tiles
    set_bkg_data(TILE_IDX_GBS_DESKTOP_FILL, HOME_GBS_DESKTOP_FILL_TILE_COUNT, gbs_desktop_fill_tile_data);
    set_bkg_data(TILE_IDX_GBS_CONTENT_FILL, GBS_WINDOW_CONTENT_FILL_TILE_COUNT, gbs_content_fill_tile_data);

    set_bkg_data(TILE_IDX_GBS_FRAME_TL, GBS_WINDOW_FRAME_TILE_COUNT, gbs_frame_tiles_data);
    
    set_bkg_data(TILE_IDX_GBS_TITLE_BAR_L, GBS_TITLE_BAR_TILE_COUNT, gbs_title_bar_tiles_data);
    set_bkg_data(TILE_IDX_GBS_TITLE_L_INACTIVE, GBS_TITLE_INACTIVE_TILE_COUNT, gbs_title_bar_inactive_tiles_data);

    set_bkg_data(TILE_IDX_GBS_DECO_MIN, GBS_WINDOW_DECORATION_TILE_COUNT, gbs_decoration_tiles_data);
    
    set_bkg_data(TILE_IDX_GBS_SCROLL_ARROW_U, GBS_SCROLLBAR_TILE_COUNT, gbs_scrollbar_tiles_data);

    // Load Minesweeper specific tiles, Paint icon tiles, Exit icon tiles etc.
    set_bkg_data((UINT8)(MINESWEEPER_TILE_VRAM_OFFSET + TILE_IDX_MS_HIDDEN), (UINT8)TILE_COUNT_MS_BASE, minesweeper_base_tiles_vram_format);
    set_bkg_data((UINT8)(MINESWEEPER_TILE_VRAM_OFFSET + TILE_IDX_MS_NUM_1), (UINT8)TILE_COUNT_MS_NUMBERS, minesweeper_number_tiles_vram_format);

    set_bkg_data((UINT8)HOME_UI_TILE_VRAM_OFFSET, (UINT8)HOME_UI_TILE_COUNT, home_ui_tiles_vram_format);

    set_bkg_data((UINT8)(HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_UI_ICON_MS), (UINT8)HOME_ICON_MS_TILE_COUNT, minesweeper_icon_tile_data);
    set_bkg_data((UINT8)(HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_UI_ICON_PAINT), (UINT8)HOME_ICON_PAINT_TILE_COUNT, paint_icon_tile_data);
    set_bkg_data((UINT8)(HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_UI_ICON_EXIT), (UINT8)HOME_ICON_EXIT_TILE_COUNT, exit_icon_tile_data);

    set_bkg_data((UINT8)(PAINT_TILE_VRAM_OFFSET + TILE_IDX_PAINT_TOOLS_BRUSH), (UINT8)PAINT_TOOLS_BRUSH_TILE_COUNT, paint_tools_tiles_vram_format);
    set_bkg_data((UINT8)(PAINT_TILE_VRAM_OFFSET + TILE_IDX_PAINT_FILL_ICON), (UINT8)PAINT_FILL_ICON_TILE_COUNT, paint_fill_icon_vram_format);

    set_sprite_data((UINT8)SPRITE_IDX_UI_CURSOR, (UINT8)UI_CURSOR_SPRITE_TILE_COUNT, ui_cursor_sprite_vram);
    set_sprite_data((UINT8)SPRITE_IDX_MS_CURSOR, (UINT8)MINESWEEPER_CURSOR_SPRITE_TILE_COUNT, minesweeper_cursor_sprite_vram);
    set_sprite_data((UINT8)SPRITE_IDX_PAINT_CURSOR, (UINT8)PAINT_CURSOR_SPRITE_TILE_COUNT, paint_cursor_sprite_vram);
}