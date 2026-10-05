/* Minimal host stand-in for GBDK's <gb/gb.h>, enough to build src/input.c in
 * unit tests. The test supplies joypad() and add_VBL(). */
#ifndef GBW_TEST_STUB_GB_H
#define GBW_TEST_STUB_GB_H

typedef unsigned char UINT8;
typedef void (*int_handler)(void);

#define J_UP     0x04U
#define J_DOWN   0x08U
#define J_LEFT   0x02U
#define J_RIGHT  0x01U
#define J_A      0x10U
#define J_B      0x20U
#define J_SELECT 0x40U
#define J_START  0x80U

/* The host test is single-threaded and calls the VBlank handler explicitly. */
#define CRITICAL

UINT8 joypad(void);
void add_VBL(int_handler handler);

#endif
