#ifndef GBW_TEXT_H
#define GBW_TEXT_H

#include <gb/gb.h>

/* Proportional system font renderer. Text is composed into a WRAM strip of
 * up to TEXT_MAX_TILES tiles, then uploaded to any VRAM bank and tile slot. */

#define TEXT_MAX_TILES 20u
#define TEXT_GLYPH_ROWS 7u

/* Two-bit palette colour indices, packed: paper | ink << 2 | box << 4. */
#define TEXT_COLORS(paper, ink, box) \
    ((UINT8)(((paper) & 3u) | (((ink) & 3u) << 2u) | (((box) & 3u) << 4u)))
#define TEXT_INK_ON_PAPER TEXT_COLORS(0u, 3u, 0u)

#define TEXT_ALIGN_LEFT   0x00u
#define TEXT_ALIGN_CENTER 0x01u
#define TEXT_ALIGN_RIGHT  0x02u
#define TEXT_ALIGN_MASK   0x03u
/* Start glyphs on pixel row 0 instead of 1, leaving row 7 free. */
#define TEXT_TOP          0x04u
/* Draw a full-width line in the ink colour on pixel row 7. */
#define TEXT_RULE_BOTTOM  0x08u
/* Draw a full-width line in colour 2 on pixel row 0. */
#define TEXT_RULE_TOP     0x10u
/* Fill a tight box around the text with the box colour (selection). */
#define TEXT_BOX          0x20u

extern const UINT8 text_font_width[95];
extern const UINT8 text_font_rows[95][TEXT_GLYPH_ROWS];

UINT8 text_width(const char *text);

/* Render into the strip. `x` is the left pixel for TEXT_ALIGN_LEFT and is
 * ignored for centre/right alignment. */
void text_render(UINT8 tiles, const char *text, UINT8 x, UINT8 colors, UINT8 flags);

/* Render centred on pixel `center_x` inside a `tiles`-wide strip, clamped so
 * the text and its selection box stay inside the strip. */
void text_render_centered_at(UINT8 tiles, const char *text, UINT8 center_x,
                             UINT8 colors, UINT8 flags);

void text_upload(UINT8 vram_bank, UINT8 first_tile, UINT8 tiles);

#endif
