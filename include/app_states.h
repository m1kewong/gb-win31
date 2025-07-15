#ifndef APP_STATES_H_GUARD
#define APP_STATES_H_GUARD

// Application States typedef enum
typedef enum {
    APP_STATE_BIOS_INIT,         // Initial state for BIOS
    APP_STATE_BIOS,              // BIOS loop
    APP_STATE_HOME_INIT,         // Initial state for Home screen (Program Manager)
    APP_STATE_HOME,              // Home screen loop
    APP_STATE_MINESWEEPER_INIT,  // Initial state for Minesweeper
    APP_STATE_MINESWEEPER,       // Minesweeper game loop
    APP_STATE_PAINT_INIT,        // Initial state for Paint
    APP_STATE_PAINT              // Paint application loop
    // Add other states or sub-states here if needed
} APP_STATE;

#endif // APP_STATES_H_GUARD
