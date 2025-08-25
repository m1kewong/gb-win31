#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdio.h>
#include <string.h> // For memcpy
#include "paint.h"
#include "tiles.h"
#include "ui.h" // For APP_STATE_HOME
#include "sound.h" // For sound effects
#include "../include/tiles.h" // For TILE_IDX constants and palette constants
#include "../include/app_states.h" // Include app_states.h

extern void gotoxy(int x, int y); // Explicit declaration
extern UINT8 current_app_state;
extern UINT8 joypad_state;

// --- Global variables for Paint ---
static PaintCursorPosition cursor;
static UINT8 current_paint_color_idx; // 0 for Black, 1 for White
static char text_buffer_paint[20]; // Buffer for printf_at

// Define the actual tile indices to be used for drawing
#define PAINT_DRAW_COLOR_BLACK TILE_IDX_EMPTY_BLACK
#define PAINT_DRAW_COLOR_WHITE TILE_IDX_EMPTY_WHITE

static const UINT8 available_draw_tiles[] = {
    PAINT_DRAW_COLOR_BLACK,
    PAINT_DRAW_COLOR_WHITE
};
#define NUM_AVAILABLE_DRAW_COLORS (sizeof(available_draw_tiles) / sizeof(available_draw_tiles[0]))

// --- Helper Functions ---

// Initialize or refresh the paint canvas background
static void paint_draw_canvas_area(void) {
    fill_bkg_rect((UINT8)PAINT_CANVAS_X, (UINT8)PAINT_CANVAS_Y, (UINT8)PAINT_CANVAS_WIDTH, (UINT8)PAINT_CANVAS_HEIGHT, (UINT8)TILE_IDX_EMPTY_WHITE); // Default to white canvas
    fill_bkg_rect_attributes((UINT8)PAINT_CANVAS_X, (UINT8)PAINT_CANVAS_Y, (UINT8)PAINT_CANVAS_WIDTH, (UINT8)PAINT_CANVAS_HEIGHT, (UINT8)PAL_IDX_BG_PAINT);
}

// Draw the Paint UI (color palette, tool icons)
static void paint_draw_ui(void) {
    fill_bkg_rect((UINT8)PAINT_UI_X, (UINT8)PAINT_UI_Y, (UINT8)PAINT_UI_WIDTH, (UINT8)PAINT_UI_HEIGHT, (UINT8)TILE_IDX_EMPTY_GREY); 
    fill_bkg_rect_attributes((UINT8)PAINT_UI_X, (UINT8)PAINT_UI_Y, (UINT8)PAINT_UI_WIDTH, (UINT8)PAINT_UI_HEIGHT, (UINT8)PAL_IDX_BG_PAINT);

    sprintf(text_buffer_paint, "Color: ");
    gotoxy(1, PAINT_UI_Y);
    printf("%s", text_buffer_paint);
    set_bkg_tile_xy(1 + (UINT8)strlen(text_buffer_paint), PAINT_UI_Y, available_draw_tiles[0]); // Black swatch
    set_bkg_attribute_xy(1 + (UINT8)strlen(text_buffer_paint), PAINT_UI_Y, PAL_IDX_BG_PAINT);
    set_bkg_tile_xy(1 + (UINT8)strlen(text_buffer_paint) + 1, PAINT_UI_Y, available_draw_tiles[1]); // White swatch
    set_bkg_attribute_xy(1 + (UINT8)strlen(text_buffer_paint) + 1, PAINT_UI_Y, PAL_IDX_BG_PAINT);
    
    // Indicate selected swatch (e.g. by drawing a small frame or different tile under it)
    if(current_paint_color_idx == 0) {
        gotoxy(1 + (UINT8)strlen(text_buffer_paint) - 1, PAINT_UI_Y);
    } else {
        gotoxy(1 + (UINT8)strlen(text_buffer_paint), PAINT_UI_Y);
    }
    printf(">");

    gotoxy(1, PAINT_UI_Y + 1);
    printf("A:Draw SEL:Swap START:Exit");
}

// Update cursor position on screen
void update_paint_cursor_sprite(void) {
    move_sprite(SPRITE_IDX_PAINT_CURSOR, cursor.x_px + 8, cursor.y_px + 16); // Offset by 8,16 for screen origin
}

// Handle drawing on the canvas
void paint_at_cursor(void) {
    // Convert pixel coordinates to tile coordinates within the canvas
    UINT8 canvas_tile_x = (cursor.x_px / 8);
    UINT8 canvas_tile_y = (cursor.y_px / 8);

    // Check bounds
    if (canvas_tile_x < PAINT_CANVAS_WIDTH && canvas_tile_y < PAINT_CANVAS_HEIGHT) {
        UINT8 screen_tile_x = PAINT_CANVAS_X + canvas_tile_x;
        UINT8 screen_tile_y = PAINT_CANVAS_Y + canvas_tile_y;

        set_bkg_tile_xy(screen_tile_x, screen_tile_y, available_draw_tiles[current_paint_color_idx]);
        set_bkg_attribute_xy(screen_tile_x, screen_tile_y, PAL_IDX_BG_PAINT);
    }
}

// --- Main Paint Application Function ---
UINT8 start_paint(void) {
    // Initialization
    // Set palettes for Paint - loaded globally in main.c
    // Ensure attributes are set correctly when drawing.

    set_sprite_tile((UINT8)SPRITE_IDX_PAINT_CURSOR, (UINT8)SPRITE_IDX_PAINT_CURSOR); // Assign tile to sprite
    set_sprite_prop((UINT8)SPRITE_IDX_PAINT_CURSOR, (UINT8)PAL_IDX_SPRITE_PAINT_CURSOR); // Assign palette to sprite

    // Initialize cursor
    cursor.x_px = (PAINT_CANVAS_WIDTH * 8) / 2;  // Center of canvas
    cursor.y_px = (PAINT_CANVAS_HEIGHT * 8) / 2; // Center of canvas
    current_paint_color_idx = 0; // Default to first color (e.g., black)

    paint_draw_canvas_area();
    paint_draw_ui();
    update_paint_cursor_sprite();

    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;

    UINT8 current_paint_state = PAINT_STATE_DRAWING; // Use a local paint state for loop control
    UINT8 previous_joypad_state = 0; // Initialize previous joypad state

    while (current_paint_state != PAINT_STATE_EXIT) {
        // Handle input
        previous_joypad_state = joypad_state;
        joypad_state = joypad();
        UINT8 pressed_joypad = (joypad_state & ~previous_joypad_state);

        if (pressed_joypad & J_START) {
            play_sound(SFX_SELECT); // Changed from play_sound_effect
            HIDE_SPRITES;
            current_paint_state = PAINT_STATE_EXIT;
        }
        if (pressed_joypad & J_SELECT) {
            current_paint_color_idx = (current_paint_color_idx + 1) % NUM_AVAILABLE_DRAW_COLORS;
            play_sound(SFX_PAINT_TOOL_SELECT); // Changed from play_sound_effect
            paint_draw_ui(); // Redraw UI to show new color selection
        }

        // Cursor Movement (simple 8px steps)
        UINT8 moved = 0;
        if ((joypad_state & J_UP) && !(previous_joypad_state & J_UP)) {
            if (cursor.y_px > 0) {cursor.y_px -= 8; moved = 1;}
        }
        if ((joypad_state & J_DOWN) && !(previous_joypad_state & J_DOWN)) {
            if (cursor.y_px < (PAINT_CANVAS_HEIGHT * 8) - 8) {cursor.y_px += 8; moved = 1;}
        }
        if ((joypad_state & J_LEFT) && !(previous_joypad_state & J_LEFT)) {
            if (cursor.x_px > 0) {cursor.x_px -= 8; moved = 1;}
        }
        if ((joypad_state & J_RIGHT) && !(previous_joypad_state & J_RIGHT)) {
            if (cursor.x_px < (PAINT_CANVAS_WIDTH * 8) - 8) {cursor.x_px += 8; moved = 1;}
        }
        if(moved) play_sound(SFX_CURSOR_MOVE);

        // Drawing
        if (joypad_state & J_A) { // Hold A to draw
            paint_at_cursor(); // paint_at_cursor could check if a sound should play
                               // to avoid rapid sound triggering. For now, play per frame.
            play_sound(SFX_PAINT_DRAW); // Changed from play_sound_effect
        }

        update_paint_cursor_sprite();

        wait_vbl_done();
    } // End of while(current_paint_state != PAINT_STATE_EXIT)
  
    return APP_STATE_HOME; // Return to home screen when exiting paint
}