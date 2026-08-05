#include <gb/gb.h>
#include <gb/cgb.h>

#include "desktop_text.h"

#define DESKTOP_TEXT_SCREEN_TILES_W 20u
#define DESKTOP_TEXT_SCREEN_TILES_H 18u
#define DESKTOP_TEXT_GLYPH_ADVANCE 4u
#define DESKTOP_TEXT_GLYPH_Y 1u

enum DesktopTextTileId {
    DESKTOP_TEXT_GB_WORKBENCH_TILE = DESKTOP_TEXT_TILE_FIRST,
    DESKTOP_TEXT_FILE_OPTIONS_HELP_TILE =
        DESKTOP_TEXT_GB_WORKBENCH_TILE + 6u,
    DESKTOP_TEXT_ACCESSORIES_TILE =
        DESKTOP_TEXT_FILE_OPTIONS_HELP_TILE + 12u,
    DESKTOP_TEXT_PAINT_TILE = DESKTOP_TEXT_ACCESSORIES_TILE + 6u,
    DESKTOP_TEXT_PIANO_TILE = DESKTOP_TEXT_PAINT_TILE + 4u,
    DESKTOP_TEXT_MEDIA_TILE = DESKTOP_TEXT_PIANO_TILE + 4u,
    DESKTOP_TEXT_GAMES_TILE = DESKTOP_TEXT_MEDIA_TILE + 4u,
    DESKTOP_TEXT_SWEEPER_TILE = DESKTOP_TEXT_GAMES_TILE + 3u,
    DESKTOP_TEXT_CANNON_TILE = DESKTOP_TEXT_SWEEPER_TILE + 4u,
    DESKTOP_TEXT_TILE_END = DESKTOP_TEXT_CANNON_TILE + 4u
};

typedef struct DesktopTextAsset {
    const char *text;
    UINT8 first_tile;
    UINT8 tile_count;
} DesktopTextAsset;

/* Original 3x5 uppercase glyphs. Each row uses the low three bits. */
static const UINT8 desktop_text_glyphs[26][5] = {
    {2u, 5u, 7u, 5u, 5u}, /* A */
    {6u, 5u, 6u, 5u, 6u}, /* B */
    {3u, 4u, 4u, 4u, 3u}, /* C */
    {6u, 5u, 5u, 5u, 6u}, /* D */
    {7u, 4u, 6u, 4u, 7u}, /* E */
    {7u, 4u, 6u, 4u, 4u}, /* F */
    {3u, 4u, 5u, 5u, 3u}, /* G */
    {5u, 5u, 7u, 5u, 5u}, /* H */
    {7u, 2u, 2u, 2u, 7u}, /* I */
    {1u, 1u, 1u, 5u, 2u}, /* J */
    {5u, 5u, 6u, 5u, 5u}, /* K */
    {4u, 4u, 4u, 4u, 7u}, /* L */
    {5u, 7u, 7u, 5u, 5u}, /* M */
    {5u, 7u, 7u, 7u, 5u}, /* N */
    {2u, 5u, 5u, 5u, 2u}, /* O */
    {6u, 5u, 6u, 4u, 4u}, /* P */
    {2u, 5u, 5u, 7u, 3u}, /* Q */
    {6u, 5u, 6u, 5u, 5u}, /* R */
    {3u, 4u, 2u, 1u, 6u}, /* S */
    {7u, 2u, 2u, 2u, 2u}, /* T */
    {5u, 5u, 5u, 5u, 7u}, /* U */
    {5u, 5u, 5u, 5u, 2u}, /* V */
    {5u, 5u, 5u, 7u, 2u}, /* W */
    {5u, 5u, 2u, 5u, 5u}, /* X */
    {5u, 5u, 2u, 2u, 2u}, /* Y */
    {7u, 1u, 2u, 4u, 7u}  /* Z */
};

static const DesktopTextAsset desktop_text_assets[] = {
    {"GB WORKBENCH", DESKTOP_TEXT_GB_WORKBENCH_TILE, 6u},
    {"FILE OPTIONS WINDOW HELP", DESKTOP_TEXT_FILE_OPTIONS_HELP_TILE, 12u},
    {"ACCESSORIES", DESKTOP_TEXT_ACCESSORIES_TILE, 6u},
    {"PAINT", DESKTOP_TEXT_PAINT_TILE, DESKTOP_TEXT_LABEL_TILES},
    {"PIANO", DESKTOP_TEXT_PIANO_TILE, DESKTOP_TEXT_LABEL_TILES},
    {"MEDIA", DESKTOP_TEXT_MEDIA_TILE, DESKTOP_TEXT_LABEL_TILES},
    {"GAMES", DESKTOP_TEXT_GAMES_TILE, 3u},
    {"SWEEPER", DESKTOP_TEXT_SWEEPER_TILE, DESKTOP_TEXT_LABEL_TILES},
    {"CANNON", DESKTOP_TEXT_CANNON_TILE, DESKTOP_TEXT_LABEL_TILES}
};

static UINT8 desktop_text_tile_buffer[16];

typedef char desktop_text_asset_count_matches_ids[
    (sizeof(desktop_text_assets) / sizeof(desktop_text_assets[0]) ==
     DESKTOP_TEXT_COUNT) ? 1 : -1
];
typedef char desktop_text_tile_count_matches_manifest[
    ((DESKTOP_TEXT_TILE_END - DESKTOP_TEXT_TILE_FIRST) ==
     DESKTOP_TEXT_TILE_COUNT) ? 1 : -1
];
typedef char desktop_text_starts_after_pointer[
    (DESKTOP_TEXT_TILE_FIRST > 120u) ? 1 : -1
];
typedef char desktop_text_stays_below_limit[
    (DESKTOP_TEXT_TILE_END <= DESKTOP_TEXT_TILE_LIMIT) ? 1 : -1
];

static UINT8 desktop_text_length(const char *text)
{
    UINT8 length = 0u;
    while (*text != '\0') {
        ++length;
        ++text;
    }
    return length;
}

static UINT8 desktop_text_glyph_row(char character, UINT8 row)
{
    if (character < 'A' || character > 'Z' || row >= 5u) return 0u;
    return desktop_text_glyphs[(UINT8)(character - 'A')][row];
}

static void desktop_text_clear_tile(void)
{
    UINT8 index;
    for (index = 0u; index != sizeof(desktop_text_tile_buffer); ++index) {
        desktop_text_tile_buffer[index] = 0u;
    }
}

static void desktop_text_set_ink_pixel(UINT8 x, UINT8 y)
{
    UINT8 mask = (UINT8)(0x80u >> x);
    UINT8 row_offset = (UINT8)(y << 1u);

    /* Both bitplanes set means palette color index 3. */
    desktop_text_tile_buffer[row_offset] |= mask;
    desktop_text_tile_buffer[(UINT8)(row_offset + 1u)] |= mask;
}

static void desktop_text_load_asset(const DesktopTextAsset *asset)
{
    UINT8 length = desktop_text_length(asset->text);
    UINT8 cell_width = (UINT8)(asset->tile_count << 3u);
    UINT8 text_width = (UINT8)(length * DESKTOP_TEXT_GLYPH_ADVANCE);
    UINT8 left_padding = (UINT8)((cell_width - text_width) >> 1u);
    UINT8 tile_index;
    UINT8 character_index;
    UINT8 row;
    UINT8 column;
    UINT8 glyph_bits;
    UINT8 pixel_x;

    for (tile_index = 0u; tile_index != asset->tile_count; ++tile_index) {
        desktop_text_clear_tile();

        for (character_index = 0u; character_index != length; ++character_index) {
            for (row = 0u; row != 5u; ++row) {
                glyph_bits = desktop_text_glyph_row(asset->text[character_index], row);
                for (column = 0u; column != 3u; ++column) {
                    if ((glyph_bits & (UINT8)(4u >> column)) == 0u) continue;
                    pixel_x = (UINT8)(left_padding +
                        character_index * DESKTOP_TEXT_GLYPH_ADVANCE + column);
                    if ((pixel_x >> 3u) == tile_index) {
                        desktop_text_set_ink_pixel(
                            (UINT8)(pixel_x & 7u),
                            (UINT8)(DESKTOP_TEXT_GLYPH_Y + row)
                        );
                    }
                }
            }
        }

        set_bkg_data(
            (UINT8)(asset->first_tile + tile_index),
            1u,
            desktop_text_tile_buffer
        );
    }
}

void desktop_text_load(void)
{
    UINT8 id;

    VBK_REG = VBK_BANK_0;
    for (id = 0u; id != (UINT8)DESKTOP_TEXT_COUNT; ++id) {
        desktop_text_load_asset(&desktop_text_assets[id]);
    }
    VBK_REG = VBK_BANK_0;
}

void desktop_text_draw(
    UINT8 tile_x,
    UINT8 tile_y,
    DesktopTextId id,
    UINT8 palette
)
{
    const DesktopTextAsset *asset;
    UINT8 count;
    UINT8 index;

    if ((UINT8)id >= (UINT8)DESKTOP_TEXT_COUNT ||
        tile_x >= DESKTOP_TEXT_SCREEN_TILES_W ||
        tile_y >= DESKTOP_TEXT_SCREEN_TILES_H) return;

    asset = &desktop_text_assets[(UINT8)id];
    count = asset->tile_count;
    if (count > (UINT8)(DESKTOP_TEXT_SCREEN_TILES_W - tile_x)) {
        count = (UINT8)(DESKTOP_TEXT_SCREEN_TILES_W - tile_x);
    }

    for (index = 0u; index != count; ++index) {
        VBK_REG = VBK_BANK_0;
        set_bkg_tile_xy(
            (UINT8)(tile_x + index),
            tile_y,
            (UINT8)(asset->first_tile + index)
        );
        set_bkg_attribute_xy(
            (UINT8)(tile_x + index),
            tile_y,
            (UINT8)(palette & 7u)
        );
    }
    VBK_REG = VBK_BANK_0;
}

UINT8 desktop_text_width_tiles(DesktopTextId id)
{
    if ((UINT8)id >= (UINT8)DESKTOP_TEXT_COUNT) return 0u;
    return desktop_text_assets[(UINT8)id].tile_count;
}
