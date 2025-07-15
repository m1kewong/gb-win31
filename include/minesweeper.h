#ifndef MINESWEEPER_H
#define MINESWEEPER_H

// It's good practice for the .c file to include gb/gb.h for these types.
// GBDK's uint8_t is typically an unsigned char.

#define MS_BOARD_WIDTH 10
#define MS_BOARD_HEIGHT 8
#define MS_NUM_MINES 10

// Screen positioning (tile coordinates) for Minesweeper elements
#define MS_SCREEN_X_MIN 1
#define MS_SCREEN_Y_MIN 1
#define MS_SCREEN_X_MAX 18
#define MS_SCREEN_Y_MAX 16

#define MS_BOARD_X_OFFSET 4  // Start board drawing a bit from the left
#define MS_BOARD_Y_OFFSET 3  // Start board drawing a bit from the top (for status)

#define MS_STATUS_X 1
#define MS_STATUS_Y 1 // Line for messages like "Mines: X" or "Game Over"
#define MS_MINE_COUNT_X_TILES 2 // Example X position for mine count text (tile units)
#define MS_MINE_COUNT_Y_TILES 1 // Example Y position for mine count text (tile units)

// Tile counts - PLACEHOLDERS, UPDATE WITH ACTUAL COUNTS
#define TILE_COUNT_MS_BASE 1    // Placeholder for hidden, mine, revealed mine, etc.
#define TILE_COUNT_MS_NUMBERS 8 // Placeholder for numbers 1-8
#define TILE_COUNT_MS_ICON 1    // Placeholder for flag icon
#define MINESWEEPER_CURSOR_SPRITE_TILE_COUNT 1 // Placeholder for cursor sprite

// Cell properties
typedef struct {
    unsigned char is_mine : 1;
    unsigned char is_revealed : 1;
    unsigned char is_flagged : 1;
    unsigned char adjacent_mines : 4; // Max 8 neighbors
} MinesweeperCell;

// Game states
#define MS_GAME_STATE_PLAYING    0
#define MS_GAME_STATE_GAME_OVER  1
#define MS_GAME_STATE_WIN        2

// Tile representations (using simple characters for now) - REPLACED BY TILE INDICES FROM tiles.h
// #define TILE_CHAR_HIDDEN       '.'
// #define TILE_CHAR_FLAG         'F'
// #define TILE_CHAR_MINE         '*'
// #define TILE_CHAR_REVEALED_MINE 'X' // When a mine is revealed (game over)
// #define TILE_CHAR_CURSOR_OPEN  '['
// #define TILE_CHAR_CURSOR_CLOSE ']'
// #define TILE_CHAR_EMPTY        ' ' // For revealed empty cells (0 adjacent mines)
// Numbers '0' through '8' will be '0' + count

// Function prototypes
UINT8 start_minesweeper(void); // Changed to return UINT8 for app state management // This is the main entry point for the game
void init_minesweeper_board(void);
void draw_minesweeper_board(void);
void place_mines(UINT8 start_x, UINT8 start_y);
void calculate_adjacent_mines(void);
void reveal_tile(UINT8 x, UINT8 y);
void reveal_all_mines(void);
void toggle_flag(UINT8 x, UINT8 y);
void check_win_condition(void);
void update_ms_cursor_sprite(void);
void update_status_display(void);

#endif