#include <gb/gb.h>
#include <stdio.h> // For gotoxy, printf, puts
#include <string.h> // For memcpy, strlen
#include "ui.h"
#include "minesweeper.h" // For start_minesweeper()
#include "paint.h"       // For start_paint()
#include "tiles.h"       // For tile and palette definitions
#include "sound.h"       // For sound effects
#include "app_states.h" // Included for APP_STATE_ definitions

extern void gotoxy(int x, int y); // Explicit declaration

#define HOME_SCREEN_NUM_ITEMS 3
#define ITEM_MINESWEEPER 0
#define ITEM_PAINT       1
#define ITEM_EXIT        2

static UINT8 home_cursor_pos = 0; // Made static
static UINT8 home_cursor_x = 5; // Made static, example screen X for cursor sprite
static UINT8 home_cursor_y_positions[HOME_SCREEN_NUM_ITEMS] = {8, 10, 12}; // Made static, example screen Y for cursor sprite, in pixels

// Helper function to fill a rectangular area with specific CGB attributes
void fill_bkg_rect_attributes(UBYTE x, UBYTE y, UBYTE w, UBYTE h, UBYTE attributes) {
    for (UBYTE iy = 0; iy < h; ++iy) {
        for (UBYTE ix = 0; ix < w; ++ix) {
            set_bkg_attribute_xy(x + ix, y + iy, attributes);
        }
    }
}

// Existing draw_text_bkg - assuming it correctly uses font tiles (0-0x7F)
// and applies the given palette_idx to them.
void draw_text_bkg(UBYTE x, UBYTE y, const char* text, UBYTE palette_idx) {
    UBYTE len = strlen(text);
    UBYTE attribute = palette_idx;
    for (UBYTE i = 0; i < len; ++i) {
        set_bkg_tile_xy(x + i, y, text[i]); 
        set_bkg_attribute_xy(x + i, y, attribute);
    }
}

// Modified draw_text_bkg to include max_width and a clear_bg option
void draw_text_bkg_clipped(UBYTE x, UBYTE y, const char* text, UBYTE palette_idx, UBYTE max_width, UBYTE bg_fill_tile, UBYTE bg_palette_idx) {
    UBYTE len = strlen(text);
    UBYTE attribute = palette_idx;
    UBYTE bg_attribute = bg_palette_idx;
    UBYTE char_to_draw;

    for (UBYTE i = 0; i < max_width; ++i) {
        if (i < len) {
            char_to_draw = text[i];
            set_bkg_tile_xy(x + i, y, char_to_draw);
            set_bkg_attribute_xy(x + i, y, attribute);
        } else {
            // Fill remaining space in max_width with bg_fill_tile and bg_palette_idx
            set_bkg_tile_xy(x + i, y, bg_fill_tile);
            set_bkg_attribute_xy(x + i, y, bg_attribute);
        }
    }
}

// New GBSWINDOWS Style drawing function for a complete window
void gbs_draw_window(UBYTE x, UBYTE y, UBYTE w, UBYTE h, const char* title, UBYTE is_active) {
    UBYTE frame_pal_attr = PAL_IDX_GBS_WINDOW_FRAME;
    UBYTE content_pal_attr = PAL_IDX_GBS_WINDOW_CONTENT;
    UBYTE title_pal_attr;
    UBYTE title_tile_l, title_tile_m, title_tile_r;

    if (is_active) {
        title_pal_attr = PAL_IDX_GBS_TITLE_ACTIVE;
        title_tile_l = HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_GBS_TITLE_BAR_L;
        title_tile_m = HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_GBS_TITLE_BAR_M;
        title_tile_r = HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_GBS_TITLE_BAR_R;
    } else {
        title_pal_attr = PAL_IDX_GBS_TITLE_INACTIVE;
        title_tile_l = HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_GBS_TITLE_L_INACTIVE;
        title_tile_m = HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_GBS_TITLE_M_INACTIVE;
        title_tile_r = HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_GBS_TITLE_R_INACTIVE;
    }

    // Draw Frame
    set_bkg_tile_xy(x, y, HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_GBS_FRAME_TL);
    set_bkg_tile_xy(x + w - 1, y, HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_GBS_FRAME_TR);
    set_bkg_tile_xy(x, y + h - 1, HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_GBS_FRAME_BL);
    set_bkg_tile_xy(x + w - 1, y + h - 1, HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_GBS_FRAME_BR);
    set_bkg_attribute_xy(x, y, frame_pal_attr);
    set_bkg_attribute_xy(x + w - 1, y, frame_pal_attr);
    set_bkg_attribute_xy(x, y + h - 1, frame_pal_attr);
    set_bkg_attribute_xy(x + w - 1, y + h - 1, frame_pal_attr);

    fill_bkg_rect(x + 1, y, w - 2, 1, HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_GBS_FRAME_T);
    fill_bkg_rect(x + 1, y + h - 1, w - 2, 1, HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_GBS_FRAME_B);
    fill_bkg_rect_attributes(x + 1, y, w - 2, 1, frame_pal_attr);
    fill_bkg_rect_attributes(x + 1, y + h - 1, w - 2, 1, frame_pal_attr);

    fill_bkg_rect(x, y + 1, 1, h - 2, HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_GBS_FRAME_L);
    fill_bkg_rect(x + w - 1, y + 1, 1, h - 2, HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_GBS_FRAME_R);
    fill_bkg_rect_attributes(x, y + 1, 1, h - 2, frame_pal_attr);
    fill_bkg_rect_attributes(x + w - 1, y + 1, 1, h - 2, frame_pal_attr);
    
    UBYTE title_bar_y = y;
    set_bkg_tile_xy(x + 1, title_bar_y, title_tile_l);
    fill_bkg_rect(x + 2, title_bar_y, w - 4, 1, title_tile_m);
    set_bkg_tile_xy(x + w - 1 - 3, title_bar_y, title_tile_r);
    fill_bkg_rect_attributes(x + 1, title_bar_y, w - 2, 1, title_pal_attr);

    UBYTE title_len = strlen(title);
    UBYTE title_text_x = x + 2; // Default start for title text
    UBYTE title_max_width = w - 2 - 1 - 3; // width - Lborder - Rborder - Rtitle_end_tile - 3 deco buttons

    // Adjust title_text_x for centering if possible
    if (title_max_width > title_len) {
        title_text_x = x + 1 + (title_max_width - title_len) / 2;
    } else {
        title_text_x = x + 1; // If title is too long, left align it after L title tile
    }
    // Ensure title_text_x is at least x + 1 (after left title bar tile)
    if (title_text_x < x + 1) title_text_x = x + 1;

    // Clear the title text area with the title bar middle tile before drawing text
    // This uses the active/inactive title bar palette.
    fill_bkg_rect(x + 1, title_bar_y, w - 2, 1, title_tile_m); // Fill entire potential text area + deco buttons area
    fill_bkg_rect_attributes(x + 1, title_bar_y, w - 2, 1, title_pal_attr); // Set palette for the fill

    // Redraw title bar end caps because fill_bkg_rect would overwrite them
    set_bkg_tile_xy(x + 1, title_bar_y, title_tile_l); // Left cap of title bar pattern
    // The right cap (title_tile_r) will be placed before deco buttons

    // Draw title text using the new clipped function
    // The background tile for clipping should be title_tile_m with title_pal_attr
    draw_text_bkg_clipped(title_text_x, title_bar_y, title, 
                          is_active ? PAL_IDX_GBS_TITLE_ACTIVE : PAL_IDX_GBS_TITLE_INACTIVE, 
                          title_max_width, 
                          title_tile_m, // Background tile for clipping/padding
                          is_active ? PAL_IDX_GBS_TITLE_ACTIVE : PAL_IDX_GBS_TITLE_INACTIVE // BG palette for clipping
                          );

    UBYTE deco_pal_attr = PAL_IDX_GBS_WINDOW_FRAME; // Or a specific deco button palette
    UBYTE deco_x_start = x + w - 1 - 3; // x of first deco button (min)
    
    // Place the right end of the title bar pattern just before the decoration buttons
    set_bkg_tile_xy(deco_x_start -1 , title_bar_y, title_tile_r);
    set_bkg_attribute_xy(deco_x_start -1, title_bar_y, title_pal_attr); // Ensure it has title bar palette

    set_bkg_tile_xy(deco_x_start, title_bar_y, HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_GBS_DECO_MIN);
    set_bkg_tile_xy(deco_x_start + 1, title_bar_y, HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_GBS_DECO_MAX);
    set_bkg_tile_xy(deco_x_start + 2, title_bar_y, HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_GBS_DECO_CLOSE);
    set_bkg_attribute_xy(deco_x_start, title_bar_y, deco_pal_attr);
    set_bkg_attribute_xy(deco_x_start + 1, title_bar_y, deco_pal_attr);
    set_bkg_attribute_xy(deco_x_start + 2, title_bar_y, deco_pal_attr);

    UBYTE content_x = x + 1;
    UBYTE content_y = y + GBS_TITLE_BAR_HEIGHT;
    UBYTE content_w = w - 2;
    UBYTE content_h = h - GBS_TITLE_BAR_HEIGHT - 1;

    fill_bkg_rect(content_x, content_y, content_w, content_h, HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_GBS_CONTENT_FILL);
    fill_bkg_rect_attributes(content_x, content_y, content_w, content_h, content_pal_attr);
}

// New GBSWINDOWS Style drawing function for an icon
void gbs_draw_icon(UBYTE screen_x, UBYTE screen_y, UBYTE icon_vram_start_idx, const char* label, UBYTE icon_gfx_pal_idx, UBYTE icon_text_pal_idx, UBYTE is_selected) {
    set_bkg_tile_xy(screen_x, screen_y, icon_vram_start_idx);
    set_bkg_tile_xy(screen_x + 1, screen_y, icon_vram_start_idx + 1);
    
    UBYTE icon_gfx_attr = icon_gfx_pal_idx;
    set_bkg_attribute_xy(screen_x, screen_y, icon_gfx_attr);
    set_bkg_attribute_xy(screen_x + 1, screen_y, icon_gfx_attr);

    UBYTE label_y = screen_y + GBS_ICON_TILE_HEIGHT;
    UBYTE label_len = strlen(label);
    UBYTE label_x = screen_x;
    if (GBS_ICON_TILE_WIDTH > label_len) {
        label_x = screen_x + (GBS_ICON_TILE_WIDTH - label_len) / 2;
    }

    UBYTE text_palette_to_use = is_selected ? PAL_IDX_GBS_ICON_TEXT_SELECTED : PAL_IDX_GBS_ICON_TEXT;
    UBYTE text_bg_fill_tile = HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_GBS_CONTENT_FILL;
    UBYTE text_bg_palette = PAL_IDX_GBS_WINDOW_CONTENT;

    if (is_selected && (gbs_palette_icon_text_selected[0] != gbs_palette_window_content[0])) {
        text_bg_fill_tile = HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_GBS_CONTENT_FILL;
        text_bg_palette = text_palette_to_use;
    }

    fill_bkg_rect(screen_x, label_y, GBS_ICON_TILE_WIDTH, 1, text_bg_fill_tile);
    fill_bkg_rect_attributes(screen_x, label_y, GBS_ICON_TILE_WIDTH, 1, text_bg_palette);

    draw_text_bkg_clipped(label_x, label_y, label, 
                          text_palette_to_use, 
                          GBS_ICON_TILE_WIDTH, 
                          text_bg_fill_tile,   
                          text_bg_palette);    
}

// Refactored function to draw the GBSWINDOWS home screen
void ui_home_draw_desktop_and_windows() {
    fill_bkg_rect(0, 0, GBS_SCREEN_TILE_WIDTH, GBS_SCREEN_TILE_HEIGHT, HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_GBS_DESKTOP_FILL);
    fill_bkg_rect_attributes(0, 0, GBS_SCREEN_TILE_WIDTH, GBS_SCREEN_TILE_HEIGHT, PAL_IDX_GBS_DESKTOP);

    gbs_draw_window(GBS_MAIN_WIN_X, GBS_MAIN_WIN_Y, GBS_MAIN_WIN_W, GBS_MAIN_WIN_H, "Program Manager", TRUE);

    const char* icon_labels[] = {"Mines", "Paint", "Exit"};
    UBYTE icon_vram_starts[] = {
        HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_GBS_ICON_MS_START,
        HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_GBS_ICON_PAINT_START,
        HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_GBS_ICON_EXIT_START
    };

    UBYTE current_icon_x = GBS_ICON_START_X;
    UBYTE current_icon_y = GBS_ICON_START_Y;
    UBYTE selected_idx = *get_selected_icon_idx_ptr();

    for (UBYTE i = 0; i < MAX_ICONS; ++i) {
        gbs_draw_icon(current_icon_x, 
                      current_icon_y, 
                      icon_vram_starts[i], 
                      icon_labels[i], 
                      PAL_IDX_GBS_ICONS_GFX,
                      PAL_IDX_GBS_ICON_TEXT,
                      i == selected_idx);

        current_icon_x += GBS_ICON_SPACING_X;
        if (current_icon_x + GBS_ICON_TILE_WIDTH > GBS_ICON_AREA_X + GBS_ICON_AREA_W) {
            current_icon_x = GBS_ICON_START_X;
            current_icon_y += GBS_ICON_SPACING_Y;
        }
    }
}

UBYTE* get_selected_icon_idx_ptr() {
    return &home_cursor_pos;
}

void ui_home_handle_input(UINT8 joypad_data, APP_STATE* current_app_state, UBYTE* selected_icon_idx) {
    static UBYTE current_selection = 0;
    UBYTE prev_selection = current_selection;

    if (joypad_data & J_LEFT) {
        if (current_selection > 0) current_selection--;
        waitpadup();
    }
    if (joypad_data & J_RIGHT) {
        if (current_selection < MAX_ICONS - 1) current_selection++;
        waitpadup();
    }

    if (prev_selection != current_selection) {
        *selected_icon_idx = current_selection;

        UBYTE prev_icon_x = GBS_ICON_START_X + (prev_selection % (GBS_ICON_AREA_W / GBS_ICON_SPACING_X)) * GBS_ICON_SPACING_X;
        UBYTE prev_icon_y = GBS_ICON_START_Y + (prev_selection / (GBS_ICON_AREA_W / GBS_ICON_SPACING_X)) * GBS_ICON_SPACING_Y;
        if (MAX_ICONS <= (GBS_ICON_AREA_W / GBS_ICON_SPACING_X)) {
             prev_icon_x = GBS_ICON_START_X + prev_selection * GBS_ICON_SPACING_X;
             prev_icon_y = GBS_ICON_START_Y;
        }

        const char* icon_labels[] = {"Mines", "Paint", "Exit"};
        UBYTE icon_vram_starts[] = {
            HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_GBS_ICON_MS_START,
            HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_GBS_ICON_PAINT_START,
            HOME_UI_TILE_VRAM_OFFSET + TILE_IDX_GBS_ICON_EXIT_START
        };

        gbs_draw_icon(prev_icon_x, prev_icon_y, icon_vram_starts[prev_selection], icon_labels[prev_selection], PAL_IDX_GBS_ICONS_GFX, PAL_IDX_GBS_ICON_TEXT, FALSE);

        UBYTE current_icon_x = GBS_ICON_START_X + (current_selection % (GBS_ICON_AREA_W / GBS_ICON_SPACING_X)) * GBS_ICON_SPACING_X;
        UBYTE current_icon_y = GBS_ICON_START_Y + (current_selection / (GBS_ICON_AREA_W / GBS_ICON_SPACING_X)) * GBS_ICON_SPACING_Y;
        if (MAX_ICONS <= (GBS_ICON_AREA_W / GBS_ICON_SPACING_X)) {
            current_icon_x = GBS_ICON_START_X + current_selection * GBS_ICON_SPACING_X;
            current_icon_y = GBS_ICON_START_Y;
        }
        gbs_draw_icon(current_icon_x, current_icon_y, icon_vram_starts[current_selection], icon_labels[current_selection], PAL_IDX_GBS_ICONS_GFX, PAL_IDX_GBS_ICON_TEXT, TRUE);
    }

    if (joypad_data & J_A) {
        waitpadup();
        switch (current_selection) {
            case 0:
                *current_app_state = APP_STATE_MINESWEEPER_INIT;
                break;
            case 1:
                *current_app_state = APP_STATE_PAINT_INIT;
                break;
            case 2:
                *current_app_state = APP_STATE_BIOS_INIT;
                break;
        }
    }
}

void ui_init() {
    DISPLAY_ON;
    SHOW_BKG;
    SHOW_SPRITES;
}

// Implementation of run_home_screen - handles the main desktop/program manager
UINT8 run_home_screen(void) {
    static UINT8 initialized = 0;
    static UINT8 selected_icon_idx = 0;
    
    if (!initialized) {
        ui_home_draw_desktop_and_windows();
        initialized = 1;
    }
    
    // Get current joypad state from global variable
    extern UINT8 joypad_state;
    UINT8 current_joypad = joypad_state;
    
    // Handle input and return next app state
    APP_STATE next_state = APP_STATE_HOME;
    ui_home_handle_input(current_joypad, &next_state, &selected_icon_idx);
    
    return next_state;
}