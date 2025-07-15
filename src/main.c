#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdio.h> 
#include "../include/app_states.h" // Include app states
#include "../include/ui.h"
#include "../include/minesweeper.h"
#include "../include/paint.h"
#include "../include/sound.h" // Ensure sound.h is included for play_sound prototype
#include "../include/tiles.h"

extern void gotoxy(int x, int y);

UINT8 current_app_state; // Declare current_app_state globally or pass as param
UINT8 joypad_state;      // Declare joypad_state globally or pass as param

void show_bios_screen(void) {
    HIDE_SPRITES;
    // Ensure background is cleared with a tile using the BIOS palette
    fill_bkg_rect((UINT8)0, (UINT8)0, (UINT8)20, (UINT8)18, (UINT8)TILE_IDX_EMPTY_BLACK); // Assuming TILE_IDX_EMPTY_BLACK is set up for this
    fill_bkg_rect_attributes((UINT8)0, (UINT8)0, (UINT8)20, (UINT8)18, (UINT8)PAL_IDX_BG_BIOS); // Use BIOS palette index

    gotoxy(6, 8);
    printf("GBC BIOS v1.0"); 
    gotoxy(3, 10);
    printf("Inspired by Win3.1");
    delay(2000); // Keep delay for BIOS screen visibility
}

int main(void) {
    SPRITES_8x8;

    load_all_tiles();    
    load_all_palettes(); // This function should set up all necessary palettes

    init_sound(); // init_sound() should be called before play_sound()
    play_sound(SFX_BOOT); 

    current_app_state = APP_STATE_BIOS;

    while(1) {
        wait_vbl_done();
        joypad_state = joypad(); // Read joypad state once per frame

        switch(current_app_state) {
            case APP_STATE_BIOS:
                show_bios_screen(); 
                current_app_state = APP_STATE_HOME; 
                break;
            case APP_STATE_HOME:
                // run_home_screen should handle its own display updates and palette usage (via attributes)
                current_app_state = run_home_screen();
                break;
            case APP_STATE_MINESWEEPER:
                // start_minesweeper should handle its own display and palette usage
                current_app_state = start_minesweeper();
                break;
            case APP_STATE_PAINT:
                // start_paint should handle its own display and palette usage
                current_app_state = start_paint();
                break;
            default:
                current_app_state = APP_STATE_HOME; // Default to home if state is unknown
                break;
        }
    }
    // return 0; // Unreachable in GBDK main loop
}