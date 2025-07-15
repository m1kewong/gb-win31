#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdio.h>
#include <string.h> // For memset
#include "minesweeper.h"
#include "sound.h" // For sound effects
#include "ui.h"    // For clear_screen_area if needed, or direct manipulation
#include "tiles.h" // For custom tile data and definitions
#include "../include/app_states.h" // Include app_states.h

extern void gotoxy(int x, int y); // Explicit declaration
extern void set_bkg_attributes_xy(UBYTE x, UBYTE y, UBYTE attributes); // Explicit declaration
extern UINT8 current_app_state;
extern UINT8 joypad_state;

// --- Custom Random Number Generator (LCG) ---
static unsigned int next_rand_seed = 1; // GBDK int is 16-bit

static void my_srand(unsigned int seed) {
    next_rand_seed = seed;
    if (next_rand_seed == 0) next_rand_seed = 1; 
}

static unsigned char my_rand(void) {
    next_rand_seed = (next_rand_seed * 25173 + 13849); // Values from Numerical Recipes (via Wikipedia)
    return (unsigned char)(next_rand_seed >> 8); 
}
// --- End Custom RNG ---

// --- Global Variables for Minesweeper ---
MinesweeperCell ms_board[MS_BOARD_HEIGHT][MS_BOARD_WIDTH];
uint8_t ms_cursor_x;
uint8_t ms_cursor_y;
uint8_t ms_mines_remaining; // Mines that are not yet flagged
uint8_t ms_flags_placed;
uint8_t ms_game_state; // PLAYING, GAME_OVER, WIN
uint8_t ms_revealed_cells_count;
uint8_t ms_total_non_mine_cells;

// Sprite number for the cursor
#define MS_CURSOR_SPRITE_ID 0

// --- Helper Function Prototypes (static) ---
static void ms_init_board(void);
static void ms_place_mines(void);
// static void ms_load_custom_tiles(void); // Tiles are loaded globally by load_all_tiles()
static void ms_update_cursor_sprite_pos(void);
static void ms_calculate_adjacent_mines(void);
static uint8_t ms_count_cell_adjacent_mines(uint8_t r, uint8_t c);
static void ms_draw_board(void);
static void ms_draw_status_bar(void);
static void ms_reveal_cell(uint8_t r, uint8_t c);
static void ms_toggle_flag(uint8_t r, uint8_t c);
static void ms_check_win_condition(void);
static void ms_game_over_sequence(uint8_t won);
static void ms_clear_game_area(void);
static void ms_update_mine_count_display(uint8_t count);

// --- Core Game Logic Implementation ---

static void ms_update_cursor_sprite_pos(void) {
    uint8_t screen_px, screen_py;
    screen_px = (MS_BOARD_X_OFFSET + ms_cursor_x) * 8 + 8;
    screen_py = (MS_BOARD_Y_OFFSET + ms_cursor_y) * 8 + 16;
    move_sprite(MS_CURSOR_SPRITE_ID, screen_px, screen_py);
}

UINT8 start_minesweeper(void) {
    uint8_t current_joypad = 0, previous_joypad = 0;

    DISPLAY_ON;
    SPRITES_8x8;
    SHOW_SPRITES;
    SHOW_BKG;

    set_sprite_tile(MS_CURSOR_SPRITE_ID, SPRITE_IDX_MS_CURSOR);

    my_srand(DIV_REG); // Seed with DIV_REG, which changes frequently
    ms_init_board();
    ms_game_state = MS_GAME_STATE_PLAYING;
    ms_update_cursor_sprite_pos();

    while(1) {
        previous_joypad = current_joypad;
        current_joypad = joypad();
        uint8_t pressed_joypad = (current_joypad & ~previous_joypad);

        if (ms_game_state == MS_GAME_STATE_PLAYING) {
            uint8_t old_cursor_x = ms_cursor_x;
            uint8_t old_cursor_y = ms_cursor_y;
            uint8_t board_updated = 0;
            uint8_t cursor_moved = 0;

            if (pressed_joypad & J_LEFT) {
                if (ms_cursor_x > 0) ms_cursor_x--;
                cursor_moved = 1;
            }
            if (pressed_joypad & J_RIGHT) {
                if (ms_cursor_x < MS_BOARD_WIDTH - 1) ms_cursor_x++;
                cursor_moved = 1;
            }
            if (pressed_joypad & J_UP) {
                if (ms_cursor_y > 0) ms_cursor_y--;
                cursor_moved = 1;
            }
            if (pressed_joypad & J_DOWN) {
                if (ms_cursor_y < MS_BOARD_HEIGHT - 1) ms_cursor_y++;
                cursor_moved = 1;
            }

            if (cursor_moved) {
                ms_update_cursor_sprite_pos();
                play_sound(SFX_CURSOR_MOVE);
            }

            if (pressed_joypad & J_A) {
                if (!ms_board[ms_cursor_y][ms_cursor_x].is_flagged && !ms_board[ms_cursor_y][ms_cursor_x].is_revealed) {
                    ms_reveal_cell(ms_cursor_y, ms_cursor_x);
                    play_sound(SFX_REVEAL_EMPTY); // Corrected: Was SFX_REVEAL_TILE, use SFX_REVEAL_EMPTY
                }
                board_updated = 1; 
            }
            if (pressed_joypad & J_B) {
                ms_toggle_flag(ms_cursor_y, ms_cursor_x);
                play_sound(SFX_PLACE_FLAG); // Corrected: Was SFX_FLAG_MINE, use SFX_PLACE_FLAG
                board_updated = 1;
            }

            if (board_updated) {
                ms_draw_board(); 
            }

        } else { 
            if (pressed_joypad & (J_A | J_START)) {
                ms_init_board();
                ms_game_state = MS_GAME_STATE_PLAYING;
                ms_update_cursor_sprite_pos();
            }
        }

        if (pressed_joypad & J_SELECT) { 
            ms_clear_game_area();
            HIDE_SPRITES;
            return APP_STATE_HOME;
        }
        wait_vbl_done();
    }
}

static void ms_clear_game_area(void) {
    unsigned char blank_tile_idx = MINESWEEPER_TILE_VRAM_OFFSET + TILE_IDX_MS_EMPTY_REVEALED;
    unsigned char blank_tile_data[1];
    blank_tile_data[0] = blank_tile_idx;

    for (uint8_t y_tile = MS_STATUS_Y; y_tile < MS_SCREEN_Y_MAX; ++y_tile) {
        for (uint8_t x_tile = MS_SCREEN_X_MIN; x_tile < MS_SCREEN_X_MAX; ++x_tile) {
            set_bkg_tiles(x_tile, y_tile, 1, 1, blank_tile_data);
        }
    }
}

static void ms_init_board(void) {
    ms_cursor_x = 0;
    ms_cursor_y = 0;
    ms_mines_remaining = MS_NUM_MINES;
    ms_flags_placed = 0;
    ms_revealed_cells_count = 0;
    ms_total_non_mine_cells = (MS_BOARD_WIDTH * MS_BOARD_HEIGHT) - MS_NUM_MINES;
    ms_game_state = MS_GAME_STATE_PLAYING;

    for (uint8_t r = 0; r < MS_BOARD_HEIGHT; ++r) {
        for (uint8_t c = 0; c < MS_BOARD_WIDTH; ++c) {
            ms_board[r][c].is_mine = 0;
            ms_board[r][c].is_revealed = 0;
            ms_board[r][c].is_flagged = 0;
            ms_board[r][c].adjacent_mines = 0;
        }
    }
    ms_place_mines();
    ms_calculate_adjacent_mines();
    ms_clear_game_area(); 
    ms_draw_board();
    ms_draw_status_bar();
}

static void ms_place_mines(void) {
    uint8_t mines_placed = 0;
    uint8_t r, c;
    while (mines_placed < MS_NUM_MINES) {
        r = my_rand() % MS_BOARD_HEIGHT;
        c = my_rand() % MS_BOARD_WIDTH;
        if (!ms_board[r][c].is_mine) {
            ms_board[r][c].is_mine = 1;
            mines_placed++;
        }
    }
}

static uint8_t ms_count_cell_adjacent_mines(uint8_t r, uint8_t c) {
    uint8_t count = 0;
    for (int8_t dr = -1; dr <= 1; ++dr) {
        for (int8_t dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) continue;
            uint8_t nr = r + dr;
            uint8_t nc = c + dc;
            if (nr < MS_BOARD_HEIGHT && nc < MS_BOARD_WIDTH) { 
                if (ms_board[nr][nc].is_mine) {
                    count++;
                }
            }
        }
    }
    return count;
}

static void ms_calculate_adjacent_mines(void) {
    for (uint8_t r = 0; r < MS_BOARD_HEIGHT; ++r) {
        for (uint8_t c = 0; c < MS_BOARD_WIDTH; ++c) {
            if (!ms_board[c][c].is_mine) {
                ms_board[r][c].adjacent_mines = ms_count_cell_adjacent_mines(r, c);
            }
        }
    }
}

static void ms_draw_status_bar(void) {
    char status_buf[20]; 
    
    fill_bkg_rect_attributes(MS_STATUS_X, MS_STATUS_Y, 19, 1, PAL_IDX_BG_MINESWEEPER);

    unsigned char space_char[1]; space_char[0] = ' '; 
    for(uint8_t i = 0; i < 19; ++i) { 
         set_bkg_tiles(MS_STATUS_X + i, MS_STATUS_Y, 1, 1, space_char);
    }

    if (ms_game_state == MS_GAME_STATE_PLAYING) {
        sprintf(status_buf, "Mines:%02u Flags:%02u", (unsigned int)ms_mines_remaining, (unsigned int)ms_flags_placed);
    } else if (ms_game_state == MS_GAME_STATE_GAME_OVER) {
        sprintf(status_buf, "GAME OVER! (A/St)");
    } else if (ms_game_state == MS_GAME_STATE_WIN) {
        sprintf(status_buf, "YOU WIN! (A/Strt)");
    }
    gotoxy(MS_STATUS_X, MS_STATUS_Y);
    printf("%s", status_buf);
}

static void ms_draw_board(void) {
    fill_bkg_rect_attributes(MS_BOARD_X_OFFSET, MS_BOARD_Y_OFFSET, MS_BOARD_WIDTH, MS_BOARD_HEIGHT, PAL_IDX_BG_MINESWEEPER);
    
    unsigned char tile_to_draw_data[1];

    for (uint8_t r = 0; r < MS_BOARD_HEIGHT; ++r) {
        for (uint8_t c = 0; c < MS_BOARD_WIDTH; ++c) {
            uint8_t screen_x = c + MS_BOARD_X_OFFSET;
            uint8_t screen_y = r + MS_BOARD_Y_OFFSET;
            uint8_t tile_idx;

            if (ms_board[r][c].is_revealed) {
                if (ms_board[r][c].is_mine) {
                    tile_idx = TILE_IDX_MS_CLICKED_MINE;
                } else if (ms_board[r][c].adjacent_mines > 0) {
                    tile_idx = TILE_IDX_MS_NUM_1 + (ms_board[r][c].adjacent_mines - 1);
                } else {
                    tile_idx = TILE_IDX_MS_EMPTY_REVEALED;
                }
            } else if (ms_board[r][c].is_flagged) {
                tile_idx = TILE_IDX_MS_FLAG;
            } else {
                if (ms_game_state == MS_GAME_STATE_GAME_OVER && ms_board[r][c].is_mine) {
                    tile_idx = TILE_IDX_MS_MINE;
                } else {
                    tile_idx = TILE_IDX_MS_HIDDEN;
                }
            }
            
            tile_to_draw_data[0] = MINESWEEPER_TILE_VRAM_OFFSET + tile_idx;
            set_bkg_tiles(screen_x, screen_y, 1, 1, tile_to_draw_data);
            set_bkg_attributes_xy(screen_x, screen_y, PAL_IDX_BG_MINESWEEPER);
        }
    }
}

static void ms_reveal_cell(uint8_t r, uint8_t c) {
    if (r >= MS_BOARD_HEIGHT || c >= MS_BOARD_WIDTH) return;
    if (ms_board[r][c].is_revealed || ms_board[r][c].is_flagged) return;

    ms_board[r][c].is_revealed = 1;

    if (ms_board[r][c].is_mine) {
        ms_game_state = MS_GAME_STATE_GAME_OVER;
        ms_game_over_sequence(0); 
        play_sound(SFX_REVEAL_MINE); // Corrected: Was SFX_GAME_OVER, use SFX_REVEAL_MINE
        return;
    }

    ms_revealed_cells_count++;

    if (ms_board[r][c].adjacent_mines == 0) {
        for (int8_t dr = -1; dr <= 1; ++dr) {
            for (int8_t dc = -1; dc <= 1; ++dc) {
                if (dr == 0 && dc == 0) continue;
                uint8_t nr = r + dr;
                uint8_t nc = c + dc;
                if (nr < MS_BOARD_HEIGHT && nc < MS_BOARD_WIDTH) {
                     ms_reveal_cell(nr, nc); 
                }
            }
        }
    } else {
        set_bkg_tile_xy(MS_BOARD_X_OFFSET + c, MS_BOARD_Y_OFFSET + r, TILE_IDX_MS_NUM_1 + (ms_board[r][c].adjacent_mines - 1));
        set_bkg_attributes_xy(MS_BOARD_X_OFFSET + c, MS_BOARD_Y_OFFSET + r, PAL_IDX_BG_MINESWEEPER);
    }
    ms_check_win_condition();
}

static void ms_toggle_flag(uint8_t r, uint8_t c) {
    if (ms_board[r][c].is_revealed) return; 

    if (ms_board[r][c].is_flagged) {
        ms_board[r][c].is_flagged = 0;
        ms_flags_placed--;
        if(ms_board[r][c].is_mine) ms_mines_remaining++; 
        play_sound(SFX_PLACE_FLAG); // Corrected: Was SFX_FLAG_MINE, use SFX_PLACE_FLAG
    } else {
        ms_board[r][c].is_flagged = 1;
        ms_flags_placed++;
        if(ms_board[r][c].is_mine) ms_mines_remaining--; 
        play_sound(SFX_PLACE_FLAG); // Corrected: Was SFX_FLAG_MINE, use SFX_PLACE_FLAG
    }
    ms_check_win_condition();
}

static void ms_check_win_condition(void) {
    if (ms_game_state != MS_GAME_STATE_PLAYING) return;

    if (ms_revealed_cells_count == ms_total_non_mine_cells) {
        ms_game_state = MS_GAME_STATE_WIN;
        ms_game_over_sequence(1); 
        play_sound(SFX_WIN_GAME); // Corrected: Was SFX_GAME_WIN, use SFX_WIN_GAME
        return;
    }
    
    if (ms_mines_remaining == 0 && ms_flags_placed == MS_NUM_MINES) {
        uint8_t correctly_flagged_mines = 0;
        for(uint8_t r_idx = 0; r_idx < MS_BOARD_HEIGHT; ++r_idx){
            for(uint8_t c_idx = 0; c_idx < MS_BOARD_WIDTH; ++c_idx){
                if(ms_board[r_idx][c_idx].is_mine && ms_board[r_idx][c_idx].is_flagged){
                    correctly_flagged_mines++;
                }
            }
        }
        if(correctly_flagged_mines == MS_NUM_MINES){
            ms_game_state = MS_GAME_STATE_WIN;
            ms_game_over_sequence(1);
            play_sound(SFX_WIN_GAME); // Corrected: Was SFX_GAME_WIN, use SFX_WIN_GAME
            return;
        }
    }
}

static void ms_game_over_sequence(uint8_t won) {
    play_sound(won ? SFX_WIN_GAME : SFX_REVEAL_MINE); // Corrected: Was SFX_GAME_WIN and SFX_GAME_OVER, use SFX_WIN_GAME and SFX_REVEAL_MINE
    if (!won) {
        for (uint8_t r = 0; r < MS_BOARD_HEIGHT; ++r) {
            for (uint8_t c = 0; c < MS_BOARD_WIDTH; ++c) {
                if (ms_board[r][c].is_mine && !ms_board[r][c].is_revealed) {
                    ms_board[r][c].is_revealed = 1; // Reveal unrevealed mines on loss
                }
            }
        }
    } else { 
        for (uint8_t r = 0; r < MS_BOARD_HEIGHT; ++r) {
            for (uint8_t c = 0; c < MS_BOARD_WIDTH; ++c) {
                if (ms_board[r][c].is_mine && !ms_board[r][c].is_flagged) {
                    ms_board[r][c].is_flagged = 1; 
                }
            }
        }
        ms_flags_placed = MS_NUM_MINES;
        ms_mines_remaining = 0;
    }
    ms_draw_board(); 
}

static void ms_update_mine_count_display(uint8_t count) {
    gotoxy(MS_MINE_COUNT_X_TILES, MS_MINE_COUNT_Y_TILES);
    fill_bkg_rect_attributes(MS_MINE_COUNT_X_TILES, MS_MINE_COUNT_Y_TILES, 7, 1, PAL_IDX_BG_MINESWEEPER); // Cover text area
    printf("Mines:%d ", count); // Added colon, ensure spacing fits
}