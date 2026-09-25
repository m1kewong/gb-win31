#include <gb/gb.h>
#include <gb/cgb.h>

#include "text.h"

#define TEXT_FIRST_CHAR 32u
#define TEXT_LAST_CHAR 126u
#define TEXT_TILE_BYTES 16u

static UINT8 text_strip[TEXT_MAX_TILES * TEXT_TILE_BYTES];

static UINT8 glyph_index(char character)
{
    UINT8 value = (UINT8)character;
    if (value < TEXT_FIRST_CHAR || value > TEXT_LAST_CHAR) value = (UINT8)'?';
    return (UINT8)(value - TEXT_FIRST_CHAR);
}

UINT8 text_width(const char *text)
{
    UINT16 width = 0u;

    while (*text != '\0') {
        width += (UINT16)text_font_width[glyph_index(*text)] + 1u;
        ++text;
    }
    if (width == 0u) return 0u;
    --width;
    return (width > 255u) ? 255u : (UINT8)width;
}

static void strip_apply(UINT8 tiles, UINT8 tile, UINT8 row, UINT8 mask, UINT8 color)
{
    UINT8 *plane;

    if (tile >= tiles || mask == 0u) return;
    plane = &text_strip[(UINT16)tile * TEXT_TILE_BYTES + (UINT8)(row << 1u)];
    if (color & 1u) plane[0] |= mask;
    else plane[0] &= (UINT8)~mask;
    if (color & 2u) plane[1] |= mask;
    else plane[1] &= (UINT8)~mask;
}

static void strip_fill(UINT8 tiles, UINT8 x0, UINT8 x1, UINT8 row0, UINT8 row1,
                       UINT8 color)
{
    UINT8 first;
    UINT8 last;
    UINT8 tile;
    UINT8 mask;
    UINT8 row;

    if (x1 <= x0) return;
    first = (UINT8)(x0 >> 3u);
    last = (UINT8)((UINT8)(x1 - 1u) >> 3u);
    for (tile = first; tile <= last && tile < tiles; ++tile) {
        mask = 0xffu;
        if (tile == first) mask &= (UINT8)(0xffu >> (x0 & 7u));
        if (tile == last) mask &= (UINT8)(0xffu << (7u - ((UINT8)(x1 - 1u) & 7u)));
        for (row = row0; row <= row1; ++row) strip_apply(tiles, tile, row, mask, color);
    }
}

static void strip_glyph(UINT8 tiles, UINT8 x, UINT8 y, UINT8 glyph, UINT8 ink)
{
    const UINT8 *rows = text_font_rows[glyph];
    UINT8 shift = (UINT8)(x & 7u);
    UINT8 tile = (UINT8)(x >> 3u);
    UINT8 row;
    UINT8 bits;
    UINT8 pixel_row;

    for (row = 0u; row != TEXT_GLYPH_ROWS; ++row) {
        bits = rows[row];
        pixel_row = (UINT8)(y + row);
        if (bits == 0u || pixel_row > 7u) continue;
        strip_apply(tiles, tile, pixel_row, (UINT8)(bits >> shift), ink);
        if (shift != 0u) {
            strip_apply(tiles, (UINT8)(tile + 1u), pixel_row,
                        (UINT8)(bits << (8u - shift)), ink);
        }
    }
}

void text_render(UINT8 tiles, const char *text, UINT8 x, UINT8 colors, UINT8 flags)
{
    UINT8 paper = (UINT8)(colors & 3u);
    UINT8 ink = (UINT8)((colors >> 2u) & 3u);
    UINT8 box = (UINT8)((colors >> 4u) & 3u);
    UINT8 area;
    UINT8 width;
    UINT8 y;
    UINT8 glyph;
    UINT16 box_end;

    if (tiles > TEXT_MAX_TILES) tiles = TEXT_MAX_TILES;
    if (tiles == 0u) return;
    area = (UINT8)(tiles << 3u);
    strip_fill(tiles, 0u, area, 0u, 7u, paper);

    width = text_width(text);
    if ((flags & TEXT_ALIGN_MASK) == TEXT_ALIGN_CENTER) {
        x = (width < area) ? (UINT8)((area - width) >> 1u) : 0u;
    } else if ((flags & TEXT_ALIGN_MASK) == TEXT_ALIGN_RIGHT) {
        x = ((UINT16)width + 1u < area) ? (UINT8)(area - width - 1u) : 0u;
    }
    y = (flags & TEXT_TOP) ? 0u : 1u;

    if (flags & TEXT_BOX) {
        box_end = (UINT16)x + width + 2u;
        if (box_end > area) box_end = area;
        strip_fill(tiles, (x >= 2u) ? (UINT8)(x - 2u) : 0u, (UINT8)box_end, 0u, 7u, box);
    }
    if (flags & TEXT_RULE_TOP) strip_fill(tiles, 0u, area, 0u, 0u, 2u);
    if (flags & TEXT_RULE_BOTTOM) strip_fill(tiles, 0u, area, 7u, 7u, ink);

    while (*text != '\0' && x < area) {
        glyph = glyph_index(*text);
        strip_glyph(tiles, x, y, glyph, ink);
        x = (UINT8)(x + text_font_width[glyph] + 1u);
        ++text;
    }
}

void text_render_centered_at(UINT8 tiles, const char *text, UINT8 center_x,
                             UINT8 colors, UINT8 flags)
{
    UINT8 width = text_width(text);
    UINT8 margin = (flags & TEXT_BOX) ? 2u : 1u;
    INT16 area = (INT16)((tiles > TEXT_MAX_TILES ? TEXT_MAX_TILES : tiles) << 3u);
    INT16 x = (INT16)center_x - (INT16)(width >> 1u);

    if (x + (INT16)width + (INT16)margin > area) x = area - (INT16)width - (INT16)margin;
    if (x < (INT16)margin) x = (INT16)margin;
    if (x < 0) x = 0;
    text_render(tiles, text, (UINT8)x, colors,
                (UINT8)(flags & (UINT8)~TEXT_ALIGN_MASK));
}

void text_upload(UINT8 vram_bank, UINT8 first_tile, UINT8 tiles)
{
    if (tiles > TEXT_MAX_TILES) tiles = TEXT_MAX_TILES;
    VBK_REG = vram_bank ? VBK_BANK_1 : VBK_BANK_0;
    set_bkg_data(first_tile, tiles, text_strip);
    VBK_REG = VBK_BANK_0;
}
