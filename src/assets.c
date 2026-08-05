#include <gb/gb.h>
#include <gb/cgb.h>

#include "assets.h"

static const palette_color_t background_palettes[] = {
    RGB(0, 25, 25), RGB(0, 15, 17), RGB(31, 31, 31), RGB(0, 0, 0),
    RGB(24, 24, 24), RGB(31, 31, 31), RGB(13, 13, 13), RGB(0, 0, 0),
    RGB(0, 0, 22), RGB(3, 9, 31), RGB(0, 0, 10), RGB(31, 31, 31),
    RGB(20, 20, 20), RGB(29, 29, 29), RGB(11, 11, 11), RGB(0, 0, 0),
    RGB(24, 24, 24), RGB(31, 27, 0), RGB(0, 23, 28), RGB(0, 0, 0),
    RGB(24, 24, 24), RGB(0, 7, 26), RGB(29, 0, 0), RGB(0, 0, 0),
    RGB(0, 0, 0), RGB(10, 10, 10), RGB(22, 22, 22), RGB(31, 31, 31),
    RGB(0, 0, 19), RGB(31, 31, 31), RGB(3, 9, 27), RGB(0, 0, 0)
};

static const palette_color_t pointer_palette[] = {
    RGB(31, 31, 31), RGB(31, 31, 31), RGB(14, 14, 14), RGB(0, 0, 0)
};

typedef char background_palette_count_must_be_eight[
    (sizeof(background_palettes) / sizeof(background_palettes[0]) == 32u) ? 1 : -1
];
typedef char static_tiles_must_not_overlap_pointer[
    (TILE_STATIC_COUNT <= TILE_POINTER_SPRITE) ? 1 : -1
];

/* Rows are five-bit glyphs for ASCII 32 through 95. */
static const UINT8 font_rows[64][7] = {
    {0,0,0,0,0,0,0}, {4,4,4,4,4,0,4}, {10,10,10,0,0,0,0},
    {10,31,10,10,31,10,0}, {4,15,20,14,5,30,4}, {24,25,2,4,8,19,3},
    {12,18,20,8,21,18,13}, {4,4,8,0,0,0,0}, {2,4,8,8,8,4,2},
    {8,4,2,2,2,4,8}, {0,10,4,31,4,10,0}, {0,4,4,31,4,4,0},
    {0,0,0,0,4,4,8}, {0,0,0,31,0,0,0}, {0,0,0,0,0,4,4},
    {1,2,4,8,16,0,0}, {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14},
    {14,17,1,2,4,8,31}, {30,1,1,14,1,1,30}, {2,6,10,18,31,2,2},
    {31,16,30,1,1,17,14}, {6,8,16,30,17,17,14}, {31,1,2,4,8,8,8},
    {14,17,17,14,17,17,14}, {14,17,17,15,1,2,12}, {0,4,4,0,4,4,0},
    {0,4,4,0,4,4,8}, {2,4,8,16,8,4,2}, {0,0,31,0,31,0,0},
    {8,4,2,1,2,4,8}, {14,17,1,2,4,0,4}, {14,17,1,13,21,21,14},
    {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30}, {14,17,16,16,16,17,14},
    {30,17,17,17,17,17,30}, {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
    {14,17,16,23,17,17,15}, {17,17,17,31,17,17,17}, {14,4,4,4,4,4,14},
    {7,2,2,2,18,18,12}, {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17}, {14,17,17,17,17,17,14},
    {30,17,17,30,16,16,16}, {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4}, {17,17,17,17,17,17,14},
    {17,17,17,17,17,10,4}, {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31}, {14,8,8,8,8,8,14},
    {16,8,4,2,1,0,0}, {14,2,2,2,2,2,14}, {4,10,17,0,0,0,0},
    {0,0,0,0,0,0,31}
};

static UINT8 tile_buffer[16];

static void tile_clear(UINT8 color)
{
    UINT8 row;
    UINT8 plane0 = (color & 1u) ? 0xffu : 0u;
    UINT8 plane1 = (color & 2u) ? 0xffu : 0u;

    for (row = 0u; row != 8u; ++row) {
        tile_buffer[row * 2u] = plane0;
        tile_buffer[row * 2u + 1u] = plane1;
    }
}

static void tile_pixel(UINT8 x, UINT8 y, UINT8 color)
{
    UINT8 mask = (UINT8)(0x80u >> x);
    UINT8 *plane0 = &tile_buffer[y * 2u];
    UINT8 *plane1 = &tile_buffer[y * 2u + 1u];

    *plane0 &= (UINT8)~mask;
    *plane1 &= (UINT8)~mask;
    if (color & 1u) *plane0 |= mask;
    if (color & 2u) *plane1 |= mask;
}

static void tile_hline(UINT8 y, UINT8 x0, UINT8 x1, UINT8 color)
{
    UINT8 x;
    for (x = x0; x <= x1; ++x) tile_pixel(x, y, color);
}

static void tile_vline(UINT8 x, UINT8 y0, UINT8 y1, UINT8 color)
{
    UINT8 y;
    for (y = y0; y <= y1; ++y) tile_pixel(x, y, color);
}

static void load_current_tile(UINT8 tile)
{
    set_bkg_data(tile, 1u, tile_buffer);
}

static void build_font(void)
{
    UINT8 glyph;
    UINT8 row;
    UINT8 bits;

    for (glyph = 0u; glyph != 64u; ++glyph) {
        tile_clear(0u);
        for (row = 0u; row != 7u; ++row) {
            bits = (UINT8)(font_rows[glyph][row] << 2u);
            tile_buffer[row * 2u] = bits;
            tile_buffer[row * 2u + 1u] = bits;
        }
        load_current_tile(glyph);
    }
}

static void build_frame_tile(UINT8 tile)
{
    tile_clear(0u);

    switch (tile) {
        case TILE_FRAME_TL:
            tile_hline(0u, 0u, 7u, 1u); tile_vline(0u, 0u, 7u, 1u);
            tile_hline(7u, 1u, 7u, 2u); tile_vline(7u, 1u, 7u, 2u);
            break;
        case TILE_FRAME_T:
            tile_hline(0u, 0u, 7u, 1u); tile_hline(7u, 0u, 7u, 2u);
            break;
        case TILE_FRAME_TR:
            tile_hline(0u, 0u, 7u, 1u); tile_vline(6u, 1u, 7u, 2u);
            tile_vline(7u, 0u, 7u, 3u); tile_hline(7u, 0u, 6u, 2u);
            break;
        case TILE_FRAME_L:
            tile_vline(0u, 0u, 7u, 1u); tile_vline(7u, 0u, 7u, 2u);
            break;
        case TILE_FRAME_R:
            tile_vline(6u, 0u, 7u, 2u); tile_vline(7u, 0u, 7u, 3u);
            break;
        case TILE_FRAME_BL:
            tile_vline(0u, 0u, 6u, 1u); tile_hline(6u, 0u, 7u, 2u);
            tile_hline(7u, 0u, 7u, 3u); tile_vline(7u, 0u, 6u, 2u);
            break;
        case TILE_FRAME_B:
            tile_hline(6u, 0u, 7u, 2u); tile_hline(7u, 0u, 7u, 3u);
            break;
        default:
            tile_hline(6u, 0u, 7u, 2u); tile_hline(7u, 0u, 7u, 3u);
            tile_vline(6u, 0u, 7u, 2u); tile_vline(7u, 0u, 7u, 3u);
            break;
    }
    load_current_tile(tile);
}

static void build_button(UINT8 tile)
{
    tile_clear(0u);
    tile_hline(0u, 0u, 7u, 1u); tile_vline(0u, 0u, 7u, 1u);
    tile_hline(7u, 0u, 7u, 3u); tile_vline(7u, 0u, 7u, 3u);

    if (tile == TILE_SYSTEM_BUTTON) {
        tile_hline(3u, 2u, 5u, 3u); tile_hline(5u, 2u, 5u, 3u);
    } else if (tile == TILE_MIN_BUTTON) {
        tile_hline(5u, 2u, 5u, 3u);
    } else {
        tile_hline(2u, 2u, 5u, 3u); tile_hline(5u, 2u, 5u, 3u);
        tile_vline(2u, 2u, 5u, 3u); tile_vline(5u, 2u, 5u, 3u);
    }
    load_current_tile(tile);
}

static void build_minesweeper_tiles(void)
{
    UINT8 tile;
    UINT8 row;
    UINT8 bits;
    UINT8 color;

    tile_clear(0u);
    tile_hline(0u, 0u, 7u, 1u); tile_vline(0u, 0u, 7u, 1u);
    tile_hline(7u, 0u, 7u, 3u); tile_vline(7u, 0u, 7u, 3u);
    load_current_tile(TILE_MS_HIDDEN);

    tile_clear(0u);
    tile_vline(3u, 2u, 6u, 3u); tile_hline(6u, 1u, 5u, 3u);
    tile_hline(2u, 3u, 6u, 2u); tile_hline(3u, 3u, 5u, 2u);
    load_current_tile(TILE_MS_FLAG);

    tile_clear(0u);
    tile_hline(3u, 1u, 6u, 3u); tile_hline(4u, 1u, 6u, 3u);
    tile_vline(3u, 1u, 6u, 3u); tile_vline(4u, 1u, 6u, 3u);
    tile_pixel(0u, 0u, 3u); tile_pixel(7u, 0u, 3u);
    tile_pixel(0u, 7u, 3u); tile_pixel(7u, 7u, 3u);
    load_current_tile(TILE_MS_MINE);

    tile_clear(2u);
    tile_hline(3u, 1u, 6u, 3u); tile_hline(4u, 1u, 6u, 3u);
    tile_vline(3u, 1u, 6u, 3u); tile_vline(4u, 1u, 6u, 3u);
    load_current_tile(TILE_MS_EXPLODED);

    for (tile = TILE_MS_NUM_1; tile <= TILE_MS_NUM_8; ++tile) {
        tile_clear(0u);
        color = (UINT8)(((tile - TILE_MS_NUM_1) % 3u) + 1u);
        for (row = 0u; row != 7u; ++row) {
            bits = (UINT8)(font_rows[(UINT8)('1' - ' ') + tile - TILE_MS_NUM_1][row] << 2u);
            tile_buffer[row * 2u] = (color & 1u) ? bits : 0u;
            tile_buffer[row * 2u + 1u] = (color & 2u) ? bits : 0u;
        }
        load_current_tile(tile);
    }
}

/*
 * Original 16 x 16 VGA-style icon art. Four pixels are packed into each
 * byte, left to right. Legend: '.' background, 'Y' yellow, 'C' cyan,
 * '#' black. Keeping the silhouettes explicit makes them readable at the
 * Game Boy Color's native resolution instead of relying on loose patterns.
 */
static const UINT8 icon_pixels[5][16][4] = {
    { /* Paint palette and diagonal brush. */
        { 0x00u, 0x00u, 0x00u, 0x00u }, /* ................ */
        { 0x00u, 0x00u, 0x00u, 0x03u }, /* ...............# */
        { 0x00u, 0x00u, 0x00u, 0x2fu }, /* .............C## */
        { 0x00u, 0x00u, 0x00u, 0x3au }, /* .............#CC */
        { 0x00u, 0xd5u, 0x70u, 0x2au }, /* ....#YYYY#...CCC */
        { 0x0du, 0x5fu, 0x57u, 0xebu }, /* ..#YYY##YYY##CC# */
        { 0x36u, 0x9fu, 0x57u, 0xabu }, /* .#YCCY##YYY#CCC# */
        { 0x36u, 0x95u, 0x56u, 0xacu }, /* .#YCCYYYYYYCCC#. */
        { 0x35u, 0x55u, 0x5au, 0xb0u }, /* .#YYYYYYYYCCC#.. */
        { 0x35u, 0xa5u, 0x7au, 0xf0u }, /* .#YYCCYYY#CC##.. */
        { 0x0du, 0xa5u, 0xeau, 0xc0u }, /* ..#YCCYY#CCC#... */
        { 0x0du, 0x55u, 0xabu, 0xc0u }, /* ..#YYYYYCCC##... */
        { 0x03u, 0x57u, 0xafu, 0xc0u }, /* ...#YYY#CC###... */
        { 0x00u, 0xd7u, 0xffu, 0x00u }, /* ....#YY#####.... */
        { 0x00u, 0x0fu, 0xfcu, 0x00u }, /* ......#####..... */
        { 0x00u, 0x00u, 0x00u, 0x00u }  /* ................ */
    },
    { /* Piano keyboard with raised black keys. */
        { 0x00u, 0x00u, 0x00u, 0x00u }, /* ................ */
        { 0x00u, 0x00u, 0x00u, 0x00u }, /* ................ */
        { 0x3fu, 0xffu, 0xffu, 0xfcu }, /* .##############. */
        { 0x3bu, 0xfbu, 0xefu, 0xecu }, /* .#C###C##C###C#. */
        { 0x37u, 0xf7u, 0xdfu, 0xdcu }, /* .#Y###Y##Y###Y#. */
        { 0x37u, 0xf7u, 0xdfu, 0xdcu }, /* .#Y###Y##Y###Y#. */
        { 0x37u, 0xf7u, 0xdfu, 0xdcu }, /* .#Y###Y##Y###Y#. */
        { 0x37u, 0xf7u, 0xdfu, 0xdcu }, /* .#Y###Y##Y###Y#. */
        { 0x37u, 0xf7u, 0xdfu, 0xdcu }, /* .#Y###Y##Y###Y#. */
        { 0x35u, 0xd7u, 0x5du, 0x5cu }, /* .#YY#YY#YY#YYY#. */
        { 0x35u, 0xd7u, 0x5du, 0x5cu }, /* .#YY#YY#YY#YYY#. */
        { 0x35u, 0xd7u, 0x5du, 0x5cu }, /* .#YY#YY#YY#YYY#. */
        { 0x35u, 0xd7u, 0x5du, 0x5cu }, /* .#YY#YY#YY#YYY#. */
        { 0x3au, 0xaau, 0xaau, 0xacu }, /* .#CCCCCCCCCCCC#. */
        { 0x3fu, 0xffu, 0xffu, 0xfcu }, /* .##############. */
        { 0x00u, 0x00u, 0x00u, 0x00u }  /* ................ */
    },
    { /* Compact cyan media deck with music note and controls. */
        { 0x00u, 0x00u, 0x00u, 0x00u }, /* ................ */
        { 0x00u, 0x00u, 0x00u, 0x00u }, /* ................ */
        { 0x3fu, 0xffu, 0xffu, 0xf0u }, /* .#############.. */
        { 0x30u, 0x00u, 0x00u, 0x30u }, /* .#...........#.. */
        { 0x32u, 0xaau, 0xaau, 0xbfu }, /* .#.CCCCCCCCCC### */
        { 0x32u, 0xaau, 0xbfu, 0xf7u }, /* .#.CCCCCC#####Y# */
        { 0x32u, 0xaau, 0xbau, 0xffu }, /* .#.CCCCCC#CC#### */
        { 0x32u, 0xaau, 0xbau, 0xf7u }, /* .#.CCCCCC#CC##Y# */
        { 0x32u, 0xaau, 0xbau, 0xb7u }, /* .#.CCCCCC#CCC#Y# */
        { 0x32u, 0xafu, 0xfau, 0xbfu }, /* .#.CCC####CCC### */
        { 0x32u, 0xbfu, 0xfau, 0xb7u }, /* .#.CC#####CCC#Y# */
        { 0x32u, 0xafu, 0xfau, 0xbfu }, /* .#.CCC####CCC### */
        { 0x35u, 0x55u, 0x55u, 0x70u }, /* .#YYYYYYYYYYY#.. */
        { 0x3fu, 0xffu, 0xffu, 0xf0u }, /* .#############.. */
        { 0x00u, 0x00u, 0x00u, 0x00u }, /* ................ */
        { 0x00u, 0x00u, 0x00u, 0x00u }  /* ................ */
    },
    { /* Sweeper grid with central mine and a flagged cell. */
        { 0x00u, 0x00u, 0x00u, 0x00u }, /* ................ */
        { 0x3fu, 0xffu, 0xffu, 0xfcu }, /* .##############. */
        { 0x30u, 0x30u, 0x30u, 0x0cu }, /* .#...#...#....#. */
        { 0x32u, 0xb2u, 0xb2u, 0xacu }, /* .#.CC#.CC#.CCC#. */
        { 0x32u, 0xb2u, 0xb2u, 0xacu }, /* .#.CC#.CC#.CCC#. */
        { 0x3fu, 0xffu, 0xffu, 0xfcu }, /* .##############. */
        { 0x3au, 0xbau, 0xfau, 0xacu }, /* .#CCC#CC##CCCC#. */
        { 0x3au, 0xbbu, 0xfeu, 0xacu }, /* .#CCC#C####CCC#. */
        { 0x3au, 0xbfu, 0x7fu, 0xacu }, /* .#CCC###Y###CC#. */
        { 0x3fu, 0xffu, 0xdfu, 0xfcu }, /* .########Y#####. */
        { 0x39u, 0x7bu, 0xfeu, 0xacu }, /* .#CYY#C####CCC#. */
        { 0x3bu, 0x7au, 0xfau, 0xacu }, /* .#C#Y#CC##CCCC#. */
        { 0x3bu, 0xbau, 0xbau, 0xacu }, /* .#C#C#CCC#CCCC#. */
        { 0x3bu, 0xbau, 0xbau, 0xacu }, /* .#C#C#CCC#CCCC#. */
        { 0x3fu, 0xffu, 0xffu, 0xfcu }, /* .##############. */
        { 0x00u, 0x00u, 0x00u, 0x00u }  /* ................ */
    },
    { /* Wheeled cannon with cyan barrel and yellow carriage. */
        { 0x00u, 0x00u, 0x00u, 0x00u }, /* ................ */
        { 0x00u, 0x00u, 0x03u, 0xffu }, /* ...........##### */
        { 0x00u, 0x00u, 0x03u, 0x03u }, /* ...........#...# */
        { 0x00u, 0x00u, 0x03u, 0xabu }, /* ...........#CCC# */
        { 0x00u, 0x00u, 0x03u, 0xabu }, /* ...........#CCC# */
        { 0x00u, 0x00u, 0x0bu, 0xffu }, /* ..........C##### */
        { 0x00u, 0x00u, 0x2au, 0xffu }, /* .........CCC#### */
        { 0x00u, 0x00u, 0xabu, 0xfcu }, /* ........CCC####. */
        { 0x00u, 0x02u, 0xafu, 0xf0u }, /* .......CCC####.. */
        { 0x00u, 0x0au, 0xbfu, 0xc0u }, /* ......CCC####... */
        { 0x00u, 0x7au, 0xffu, 0x00u }, /* ....Y#CC####.... */
        { 0x01u, 0x7fu, 0xfdu, 0x70u }, /* ...YY######YY#.. */
        { 0x0du, 0x7eu, 0xadu, 0x70u }, /* ..#YY##CCC#YY#.. */
        { 0x0fu, 0xfeu, 0xefu, 0xf0u }, /* ..#####C#C####.. */
        { 0x00u, 0x0eu, 0xacu, 0x00u }, /* ......#CCC#..... */
        { 0x00u, 0x0fu, 0xfcu, 0x00u }  /* ......#####..... */
    }
};

static UINT8 icon_pixel(UINT8 icon, UINT8 x, UINT8 y)
{
    UINT8 packed = icon_pixels[icon][y][x >> 2u];
    UINT8 shift = (UINT8)((3u - (x & 3u)) << 1u);
    return (UINT8)((packed >> shift) & 3u);
}

static void build_icons(void)
{
    UINT8 icon;
    UINT8 tile_x;
    UINT8 tile_y;
    UINT8 x;
    UINT8 y;
    UINT8 color;
    UINT8 tile;

    for (icon = 0u; icon != 5u; ++icon) {
        for (tile_y = 0u; tile_y != 2u; ++tile_y) {
            for (tile_x = 0u; tile_x != 2u; ++tile_x) {
                tile_clear(0u);
                for (y = 0u; y != 8u; ++y) {
                    for (x = 0u; x != 8u; ++x) {
                        color = icon_pixel(icon, (UINT8)(tile_x * 8u + x), (UINT8)(tile_y * 8u + y));
                        tile_pixel(x, y, color);
                    }
                }
                tile = (UINT8)(TILE_ICON_FIRST + icon * 4u + tile_y * 2u + tile_x);
                load_current_tile(tile);
            }
        }
    }
}

static void build_misc_tiles(void)
{
    tile_clear(0u); load_current_tile(TILE_BLANK);
    tile_clear(3u); load_current_tile(TILE_SOLID);

    tile_clear(0u); tile_hline(3u, 1u, 6u, 3u); tile_vline(3u, 1u, 6u, 3u);
    load_current_tile(TILE_CURSOR_MARK);

    tile_clear(0u); tile_vline(0u, 0u, 7u, 3u); tile_vline(7u, 0u, 7u, 3u);
    tile_hline(7u, 0u, 7u, 3u); load_current_tile(TILE_KEY_WHITE);
    tile_clear(1u); tile_vline(0u, 0u, 7u, 3u); tile_vline(7u, 0u, 7u, 3u);
    tile_hline(7u, 0u, 7u, 3u); load_current_tile(TILE_KEY_WHITE_ACTIVE);
    tile_clear(3u); tile_hline(7u, 0u, 7u, 2u); load_current_tile(TILE_KEY_BLACK);

    tile_clear(0u); tile_hline(6u, 1u, 6u, 2u); tile_hline(5u, 2u, 5u, 2u);
    tile_vline(4u, 2u, 5u, 3u); load_current_tile(TILE_CANNON);
    tile_clear(0u); tile_pixel(3u, 3u, 2u); tile_pixel(4u, 3u, 2u);
    tile_pixel(3u, 4u, 2u); tile_pixel(4u, 4u, 2u); load_current_tile(TILE_TARGET);
}

static void build_pointer(void)
{
    static const UINT8 pointer_tiles[32] = {
        0x80u, 0x80u,
        0xc0u, 0xc0u,
        0xe0u, 0xa0u,
        0xf0u, 0x90u,
        0xf8u, 0x88u,
        0xfcu, 0x84u,
        0xfeu, 0x82u,
        0xf8u, 0xb8u,
        0xf8u, 0xe8u,
        0x38u, 0x28u,
        0x38u, 0x28u,
        0x38u, 0x38u,
        0x00u, 0x00u,
        0x00u, 0x00u,
        0x00u, 0x00u,
        0x00u, 0x00u
    };

    set_sprite_data(TILE_POINTER_SPRITE, TILE_POINTER_SPRITE_COUNT, pointer_tiles);
}

void assets_load(void)
{
    UINT8 tile;

    VBK_REG = VBK_TILES;
    build_font();
    build_misc_tiles();
    for (tile = TILE_FRAME_TL; tile <= TILE_FRAME_BR; ++tile) build_frame_tile(tile);
    build_button(TILE_SYSTEM_BUTTON);
    build_button(TILE_MIN_BUTTON);
    build_button(TILE_MAX_BUTTON);
    build_minesweeper_tiles();
    build_icons();
    build_pointer();
    VBK_REG = VBK_TILES;

    set_bkg_palette(0u, 8u, background_palettes);
    set_sprite_palette(0u, 1u, pointer_palette);
}

UINT8 assets_font_tile(char c)
{
    UINT8 value = (UINT8)c;
    if (value >= (UINT8)'a' && value <= (UINT8)'z') value = (UINT8)(value - 32u);
    if (value < 32u || value > 95u) value = (UINT8)'?';
    return (UINT8)(value - 32u);
}
