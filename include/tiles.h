#ifndef DESKTOP_TILES_H
#define DESKTOP_TILES_H

extern const unsigned char desktop_tiles[];

// Added for Minesweeper
extern const unsigned char minesweeper_cursor_sprite_vram[]; // Corrected name

#define MINESWEEPER_TILE_COUNT 13
#define MINESWEEPER_CURSOR_TILE_COUNT 1

// VRAM offset for loading Minesweeper custom tiles.
// Assumes default font uses tiles before this offset.
#define MINESWEEPER_TILE_VRAM_OFFSET 0x50 // Tile 80

// Indices for specific tiles within minesweeper_tile_data
// These will be added to MINESWEEPER_TILE_VRAM_OFFSET when used with set_bkg_tiles
#define TILE_IDX_MS_HIDDEN          0
#define TILE_IDX_MS_FLAG            1
#define TILE_IDX_MS_MINE            2
#define TILE_IDX_MS_CLICKED_MINE    3
#define TILE_IDX_MS_EMPTY_REVEALED  4 // For 0 adjacent mines
#define TILE_IDX_MS_NUM_1           5
#define TILE_IDX_MS_NUM_2           6
#define TILE_IDX_MS_NUM_3           7
#define TILE_IDX_MS_NUM_4           8
#define TILE_IDX_MS_NUM_5           9
#define TILE_IDX_MS_NUM_6           10
#define TILE_IDX_MS_NUM_7           11
#define TILE_IDX_MS_NUM_8           12

// --- GBC Color Palettes ---
// Helper macro for defining GBC RGB colors (5 bits per channel)
// #define RGB(r,g,b) (uint16_t)((r)|((g)<<5)|((b)<<10)) // Use GBDK's version from <gb/cgb.h>

// Palette indices for Backgrounds
#define PAL_IDX_BG_BIOS 0
#define PAL_IDX_BG_HOME_DESKTOP 0 // Home screen main background (now dark blue)
#define PAL_IDX_BG_MAIN_FRAME 1   // New: For main "GBSWINDOWS" window frame
#define PAL_IDX_BG_SUB_FRAME 2    // New: For "Accessories" & "Games" window frames
#define PAL_IDX_BG_SUB_CONTENT 3  // New: For content area of sub-windows (light grey)
#define PAL_IDX_BG_HOME_ICONS 4   // For all icons (generic 4-color palette)
#define PAL_IDX_BG_MINESWEEPER 5  // Shifted index
#define PAL_IDX_BG_PAINT 6        // This might need re-evaluation based on new scheme

// New GBSWINDOWS Style Palette Indices
#define PAL_IDX_GBS_DESKTOP 0
#define PAL_IDX_GBS_WINDOW_FRAME 1      // Used for window borders and decoration buttons
#define PAL_IDX_GBS_TITLE_ACTIVE 2      // For active window title bars
#define PAL_IDX_GBS_WINDOW_CONTENT 3    // For the content area of windows
#define PAL_IDX_GBS_ICONS_GFX 4         // For program icon graphics
#define PAL_IDX_GBS_ICON_TEXT 5         // For text labels under icons
#define PAL_IDX_GBS_TITLE_INACTIVE 6    // For inactive window title bars
#define PAL_IDX_GBS_SCROLLBAR 7         // For scrollbar elements (if used)
#define PAL_IDX_GBS_ICON_TEXT_SELECTED 8 // For selected icon text

// Palette indices for Sprites (OBP0 - OBP7)
#define PAL_IDX_SPRITE_UI_CURSOR 0
#define PAL_IDX_SPRITE_MS_CURSOR 1
#define PAL_IDX_SPRITE_PAINT_CURSOR 2
// Reserve 3-7 for other sprites

// Sprite Tile Indices (absolute indices in OAM/VRAM for sprites)
#define SPRITE_IDX_UI_CURSOR 0
#define UI_CURSOR_SPRITE_TILE_COUNT 1 // Assuming 1 tile for the UI cursor
#define SPRITE_IDX_MS_CURSOR 1 // Correct definition for Minesweeper cursor sprite index
// MINESWEEPER_CURSOR_TILE_COUNT is already defined above
#define SPRITE_IDX_PAINT_CURSOR 2
#define PAINT_CURSOR_SPRITE_TILE_COUNT 1 // Assuming 1 tile for the paint cursor

// --- General Purpose Tiles (e.g., for filling areas) ---
// These are loaded early and can be reused.
// Their VRAM locations are defined here.
#define TILE_IDX_BLANK                  0x6F // Added missing definition
#define TILE_IDX_EMPTY_BLACK            0x70 // Fixed hex value
#define TILE_IDX_EMPTY_TEAL             0x71 // Fixed hex value
#define TILE_IDX_EMPTY_GREY             0x72 // Fixed hex value
#define TILE_IDX_PAINT_CANVAS_FILL      0x73 // Fixed hex value
#define TILE_IDX_EMPTY_WHITE            0x74 // Fixed hex value

extern const unsigned char tile_data_empty_black[];
extern const unsigned char tile_data_empty_teal[];
extern const unsigned char tile_data_empty_grey[];
extern const unsigned char tile_data_paint_canvas_fill[];
extern const unsigned char tile_data_empty_white[];

// Extern declarations for palettes - ensure these EXACTLY match definitions
extern const UWORD bios_palette_win31[];

// New GBSWINDOWS Style Palette Externs
extern const UWORD gbs_palette_desktop[];
extern const UWORD gbs_palette_window_frame[];
extern const UWORD gbs_palette_title_active[];
extern const UWORD gbs_palette_window_content[];
extern const UWORD gbs_palette_icons_gfx[];
extern const UWORD gbs_palette_icon_text[];
extern const UWORD gbs_palette_title_inactive[];
extern const UWORD gbs_palette_scrollbar[];
extern const UWORD gbs_palette_icon_text_selected[]; // New extern

extern const UWORD ui_cursor_sprite_palette_win31[];
extern const UWORD minesweeper_bg_palette[];
extern const UWORD minesweeper_numbers_palette[];
extern const UWORD minesweeper_status_palette[];
extern const UWORD minesweeper_cursor_sprite_palette[];
extern const UWORD paint_bg_palette[];
extern const UWORD paint_cursor_sprite_palette[];

// Function Prototypes
void load_all_tiles(void);
void load_all_palettes(void); // Renamed for clarity if it wasn't already

// --- Home Screen UI Tiles (GBSWINDOWS Style) ---
// Base VRAM offset for home screen UI tiles.
// Ensure this is after system font and any other global tiles like Minesweeper game tiles.
// Font: ~0x00-0x4F (80 tiles)
// Minesweeper Game Tiles: MINESWEEPER_TILE_VRAM_OFFSET (0x50) + MINESWEEPER_TILE_COUNT (13) = up to 0x5C
// Let's keep HOME_UI_TILE_VRAM_OFFSET at 0x80 as a safe start.
#define HOME_UI_TILE_VRAM_OFFSET    0x80

// Tile indices for GBSWINDOWS home screen UI elements.
// These are relative to HOME_UI_TILE_VRAM_OFFSET.

// Desktop
#define TILE_IDX_GBS_DESKTOP_FILL           0
#define HOME_GBS_DESKTOP_FILL_TILE_COUNT    1

// Window Frame Tiles (reusable for main and sub-windows if design is consistent)
// These are the 8 border pieces.
#define TILE_IDX_GBS_FRAME_TL                (TILE_IDX_GBS_DESKTOP_FILL + HOME_GBS_DESKTOP_FILL_TILE_COUNT)
#define TILE_IDX_GBS_FRAME_T                 (TILE_IDX_GBS_FRAME_TL + 1)
#define TILE_IDX_GBS_FRAME_TR                (TILE_IDX_GBS_FRAME_T + 1)
#define TILE_IDX_GBS_FRAME_L                 (TILE_IDX_GBS_FRAME_TR + 1)
#define TILE_IDX_GBS_FRAME_R                 (TILE_IDX_GBS_FRAME_L + 1)
#define TILE_IDX_GBS_FRAME_BL                (TILE_IDX_GBS_FRAME_R + 1)
#define TILE_IDX_GBS_FRAME_B                 (TILE_IDX_GBS_FRAME_BL + 1)
#define TILE_IDX_GBS_FRAME_BR                (TILE_IDX_GBS_FRAME_B + 1)
#define GBS_WINDOW_FRAME_TILE_COUNT          8

// Title Bar Specific Tiles (excluding text, which uses font tiles)
#define TILE_IDX_GBS_TITLE_BAR_L             (TILE_IDX_GBS_FRAME_BR + 1) // Left end of title bar pattern
#define TILE_IDX_GBS_TITLE_BAR_M             (TILE_IDX_GBS_TITLE_BAR_L + 1) // Middle, repeatable part of title bar pattern
#define TILE_IDX_GBS_TITLE_BAR_R             (TILE_IDX_GBS_TITLE_BAR_M + 1) // Right end of title bar pattern
#define GBS_TITLE_BAR_TILE_COUNT             3

// Window Decoration Button Tiles (for title bars)
#define TILE_IDX_GBS_DECO_MIN                (TILE_IDX_GBS_TITLE_BAR_R + 1)
#define TILE_IDX_GBS_DECO_MAX                (TILE_IDX_GBS_DECO_MIN + 1)
#define TILE_IDX_GBS_DECO_CLOSE              (TILE_IDX_GBS_DECO_MAX + 1)
#define GBS_WINDOW_DECORATION_TILE_COUNT     3

// Window Content Fill Tile
#define TILE_IDX_GBS_CONTENT_FILL            (TILE_IDX_GBS_DECO_CLOSE + 1)
#define GBS_WINDOW_CONTENT_FILL_TILE_COUNT   1

// Inactive Title Bar Tiles (mirrors active title bar structure)
#define TILE_IDX_GBS_TITLE_L_INACTIVE        (TILE_IDX_GBS_CONTENT_FILL + GBS_WINDOW_CONTENT_FILL_TILE_COUNT)
#define TILE_IDX_GBS_TITLE_M_INACTIVE        (TILE_IDX_GBS_TITLE_L_INACTIVE + 1)
#define TILE_IDX_GBS_TITLE_R_INACTIVE        (TILE_IDX_GBS_TITLE_M_INACTIVE + 1)
#define GBS_TITLE_INACTIVE_TILE_COUNT        3

// Scrollbar Tiles
#define TILE_IDX_GBS_SCROLL_ARROW_U          (TILE_IDX_GBS_TITLE_R_INACTIVE + 1)
#define TILE_IDX_GBS_SCROLL_ARROW_D          (TILE_IDX_GBS_SCROLL_ARROW_U + 1)
#define TILE_IDX_GBS_SCROLL_THUMB_M          (TILE_IDX_GBS_SCROLL_ARROW_D + 1) // Placeholder for more thumb parts
#define TILE_IDX_GBS_SCROLL_TRACK_V          (TILE_IDX_GBS_SCROLL_THUMB_M + 1) // Placeholder for H track
#define GBS_SCROLLBAR_TILE_COUNT             4 // Current count for U, D, Thumb_M, Track_V

// Program Icons (placeholders, actual data loaded separately but VRAM allocated here)
#define TILE_IDX_GBS_ICON_MS_START           (TILE_IDX_GBS_SCROLL_TRACK_V + 1)
// Assuming icons are still 2 tiles (16x8)
#define GBS_ICON_MS_TILE_COUNT               2 
#define TILE_IDX_GBS_ICON_PAINT_START        (TILE_IDX_GBS_ICON_MS_START + GBS_ICON_MS_TILE_COUNT)
#define GBS_ICON_PAINT_TILE_COUNT            2
#define TILE_IDX_GBS_ICON_EXIT_START         (TILE_IDX_GBS_ICON_PAINT_START + GBS_ICON_PAINT_TILE_COUNT)
#define GBS_ICON_EXIT_TILE_COUNT             2

// Total count of unique UI tiles defined in home_ui_tiles_vram_format (excluding program icons data, but including their VRAM slots)
#define HOME_GBS_UI_TILE_COUNT  (HOME_GBS_DESKTOP_FILL_TILE_COUNT + \
                                 GBS_WINDOW_FRAME_TILE_COUNT + \
                                 GBS_TITLE_BAR_TILE_COUNT + \
                                 GBS_WINDOW_DECORATION_TILE_COUNT + \
                                 GBS_WINDOW_CONTENT_FILL_TILE_COUNT + \
                                 GBS_TITLE_INACTIVE_TILE_COUNT + \
                                 GBS_SCROLLBAR_TILE_COUNT + \
                                 GBS_ICON_MS_TILE_COUNT + /* Placeholder slots */ \
                                 GBS_ICON_PAINT_TILE_COUNT + /* Placeholder slots */ \
                                 GBS_ICON_EXIT_TILE_COUNT /* Placeholder slots */)

// Extern declarations for the new UI tile data arrays
extern const unsigned char gbs_desktop_fill_tile_data[];
extern const unsigned char gbs_frame_tiles_data[]; // Array containing all 8 frame tiles
extern const unsigned char gbs_title_bar_tiles_data[]; // Array for L, M, R active title bar pattern tiles
extern const unsigned char gbs_title_bar_inactive_tiles_data[]; // Added missing definition
extern const unsigned char gbs_decoration_tiles_data[]; // Added missing definition
extern const unsigned char gbs_scrollbar_tiles_data[]; // Added missing definition
extern const unsigned char gbs_content_fill_tile_data[];

// Add missing tile data definitions
extern const unsigned char minesweeper_base_tiles_vram_format[];
extern const unsigned char minesweeper_number_tiles_vram_format[];
extern const unsigned char home_ui_tiles_vram_format[];
extern const unsigned char ui_cursor_sprite_vram[];

// Missing icon-related definitions
#define TILE_IDX_UI_ICON_MS     TILE_IDX_GBS_ICON_MS_START
#define TILE_IDX_UI_ICON_PAINT  TILE_IDX_GBS_ICON_PAINT_START  
#define TILE_IDX_UI_ICON_EXIT   TILE_IDX_GBS_ICON_EXIT_START
#define HOME_ICON_MS_TILE_COUNT GBS_ICON_MS_TILE_COUNT
#define HOME_ICON_PAINT_TILE_COUNT GBS_ICON_PAINT_TILE_COUNT
#define HOME_ICON_EXIT_TILE_COUNT GBS_ICON_EXIT_TILE_COUNT
#define HOME_UI_TILE_COUNT HOME_GBS_UI_TILE_COUNT

// Externs for inactive title bar tiles
extern const unsigned char gbs_title_L_inactive_tile_data[];
extern const unsigned char gbs_title_M_inactive_tile_data[];
extern const unsigned char gbs_title_R_inactive_tile_data[];

// Externs for scrollbar tiles
extern const unsigned char gbs_scroll_arrow_U_tile_data[];
extern const unsigned char gbs_scroll_arrow_D_tile_data[];
extern const unsigned char gbs_scroll_thumb_M_tile_data[];
extern const unsigned char gbs_scroll_track_V_tile_data[];

// Extern declarations for icon tile data (ensure these are correct)
extern const unsigned char minesweeper_icon_tile_data[];
extern const unsigned char paint_icon_tile_data[];
extern const unsigned char exit_icon_tile_data[];

// Paint Cursor Sprite
// Assumes sprite tile patterns for Minesweeper (0) and UI (1) are already loaded or managed.
extern const unsigned char paint_cursor_sprite_vram[]; // Declaration for sprite data in src/tiles.c

// --- Paint Application Specific Tiles ---
// VRAM offset for loading Paint-specific tiles (tools, fill icons, etc.)
// This should be after HOME_UI_TILE_VRAM_OFFSET + HOME_UI_TILE_COUNT
// HOME_UI_TILE_VRAM_OFFSET (0x80) + HOME_UI_TILE_COUNT (approx 31+6 = 37 tiles -> ~0xA5)
// Let's set Paint VRAM offset to 0xB0 for safety.
#define PAINT_TILE_VRAM_OFFSET 0xB0

// Indices relative to PAINT_TILE_VRAM_OFFSET
#define TILE_IDX_PAINT_TOOLS_BRUSH  0
#define PAINT_TOOLS_BRUSH_TILE_COUNT 1
#define TILE_IDX_PAINT_FILL_ICON    (TILE_IDX_PAINT_TOOLS_BRUSH + PAINT_TOOLS_BRUSH_TILE_COUNT)
#define PAINT_FILL_ICON_TILE_COUNT  1
// Add more paint tool/UI tiles here...

#define PAINT_TOTAL_SPECIFIC_TILES (PAINT_TOOLS_BRUSH_TILE_COUNT + PAINT_FILL_ICON_TILE_COUNT)

// Extern declarations for Paint-specific tile data arrays
extern const UBYTE paint_tools_tiles_vram_format[]; // Ensure this matches definition
extern const UBYTE paint_fill_icon_vram_format[];   // Ensure this matches definition

#endif // DESKTOP_TILES_H