#pragma bank 255

#include <gb/gb.h>

#include "assets.h"
#include "audio.h"
#include "desktop.h"
#include "text.h"
#include "ui.h"

/* Program Manager text lives in persistent bank-0 tiles so application
 * windows can redraw the desktop behind them without re-rendering it. */
#define DESKTOP_TEXT_FIRST (TILE_POINTER_SPRITE + TILE_POINTER_SPRITE_COUNT)

enum DesktopTextTile {
    DT_TITLE = DESKTOP_TEXT_FIRST,
    DT_MENU = DT_TITLE + 8u,
    DT_ACCESSORIES = DT_MENU + 18u,
    DT_GAMES = DT_ACCESSORIES + 8u,
    DT_CAPTIONS = DT_GAMES + 4u,
    DT_END = DT_CAPTIONS + 28u
};

typedef char desktop_text_fits_bank_zero[(DT_END <= 256u) ? 1 : -1];

#define DESKTOP_GROUP_X 1u
#define DESKTOP_GROUP_W 18u
#define DESKTOP_ACCESSORIES_Y 2u
#define DESKTOP_ACCESSORIES_H 8u
#define DESKTOP_GAMES_Y 10u
#define DESKTOP_GAMES_H 7u
#define DESKTOP_NO_ICON 0xffu

typedef struct DesktopIcon {
    UINT8 icon_x;
    UINT8 icon_y;
    UINT8 caption_x;
    UINT8 caption_y;
    UINT8 caption_tiles;
    UINT8 caption_tile;
    UINT8 center;
    UINT8 icon_tile;
    const char *label;
    AppState app;
    UINT8 group;
} DesktopIcon;

static const DesktopIcon desktop_icons[] = {
    {3u, 4u, 2u, 7u, 5u, DT_CAPTIONS, 32u, TILE_ICON_PAINT, "Paint", APP_PAINT, 0u},
    {9u, 4u, 7u, 7u, 6u, DT_CAPTIONS + 5u, 80u, TILE_ICON_PIANO, "Piano", APP_PIANO, 0u},
    {15u, 4u, 13u, 7u, 5u, DT_CAPTIONS + 11u, 128u, TILE_ICON_MEDIA, "Media", APP_MEDIA, 0u},
    {5u, 12u, 3u, 14u, 6u, DT_CAPTIONS + 16u, 48u, TILE_ICON_SWEEPER, "Sweeper", APP_SWEEPER, 1u},
    {13u, 12u, 11u, 14u, 6u, DT_CAPTIONS + 22u, 112u, TILE_ICON_CANNON, "Cannon", APP_CANNON, 1u}
};

#define DESKTOP_ICON_COUNT (sizeof(desktop_icons) / sizeof(desktop_icons[0]))

static UINT8 selected_icon;
static UINT8 active_group;
static UINT8 desktop_focused;

static void render_persistent(UINT8 first_tile, UINT8 tiles, const char *text,
                              UINT8 center_x, UINT8 flags)
{
    text_render_centered_at(tiles, text, center_x, TEXT_INK_ON_PAPER, flags);
    text_upload(0u, first_tile, tiles);
}

static void render_caption(UINT8 index, UINT8 selected)
{
    const DesktopIcon *icon = &desktop_icons[index];
    UINT8 center = (UINT8)(icon->center - (UINT8)(icon->caption_x << 3u));

    if (selected) {
        text_render_centered_at(icon->caption_tiles, icon->label, center,
                                TEXT_COLORS(3u, 3u, 0u), TEXT_BOX);
    } else {
        text_render_centered_at(icon->caption_tiles, icon->label, center,
                                TEXT_INK_ON_PAPER, 0u);
    }
    text_upload(0u, icon->caption_tile, icon->caption_tiles);
}

static void render_text(void)
{
    UINT8 i;

    render_persistent(DT_TITLE, 8u, "GB Workbench", 32u, 0u);
    text_render(18u, "File  Options  Window  Help", 2u, TEXT_INK_ON_PAPER,
                (UINT8)(TEXT_TOP | TEXT_RULE_BOTTOM));
    text_upload(0u, DT_MENU, 18u);
    render_persistent(DT_ACCESSORIES, 8u, "Accessories", 32u, 0u);
    render_persistent(DT_GAMES, 4u, "Games", 16u, 0u);
    for (i = 0u; i != DESKTOP_ICON_COUNT; ++i) {
        render_caption(i, (UINT8)(i == selected_icon));
    }
}

static void map_run(UINT8 x, UINT8 y, UINT8 first_tile, UINT8 count, UINT8 palette)
{
    UINT8 i;
    for (i = 0u; i != count; ++i) {
        ui_set_tile((UINT8)(x + i), y, (UINT8)(first_tile + i), palette);
    }
}

static UINT8 group_palette(UINT8 group)
{
    return (desktop_focused && active_group == group) ?
        PAL_TITLE_ACTIVE : PAL_TITLE_INACTIVE;
}

static void draw_group_titles(void)
{
    UINT8 palette = group_palette(0u);
    ui_fill(3u, DESKTOP_ACCESSORIES_Y, 13u, 1u, TILE_BLANK, palette);
    map_run(6u, DESKTOP_ACCESSORIES_Y, DT_ACCESSORIES, 8u, palette);

    palette = group_palette(1u);
    ui_fill(3u, DESKTOP_GAMES_Y, 13u, 1u, TILE_BLANK, palette);
    map_run(8u, DESKTOP_GAMES_Y, DT_GAMES, 4u, palette);
}

static void draw_caption(UINT8 index)
{
    const DesktopIcon *icon = &desktop_icons[index];
    map_run(icon->caption_x, icon->caption_y, icon->caption_tile,
            icon->caption_tiles,
            (index == selected_icon) ? PAL_TITLE_ACTIVE : PAL_WINDOW);
}

static void draw_icon(UINT8 index)
{
    const DesktopIcon *icon = &desktop_icons[index];

    ui_set_tile(icon->icon_x, icon->icon_y, icon->icon_tile, PAL_ICON);
    ui_set_tile((UINT8)(icon->icon_x + 1u), icon->icon_y,
                (UINT8)(icon->icon_tile + 1u), PAL_ICON);
    ui_set_tile(icon->icon_x, (UINT8)(icon->icon_y + 1u),
                (UINT8)(icon->icon_tile + 2u), PAL_ICON);
    ui_set_tile((UINT8)(icon->icon_x + 1u), (UINT8)(icon->icon_y + 1u),
                (UINT8)(icon->icon_tile + 3u), PAL_ICON);
    draw_caption(index);
}

static void draw_program_manager(void)
{
    UINT8 i;

    ui_window(0u, 0u, SCREEN_TILES_W, SCREEN_TILES_H, (const char *)0,
              desktop_focused, UI_CLIENT_WHITE);
    map_run(6u, 0u, DT_TITLE, 8u,
            desktop_focused ? PAL_TITLE_ACTIVE : PAL_TITLE_INACTIVE);
    map_run(1u, 1u, DT_MENU, 18u, PAL_WINDOW);

    ui_window(DESKTOP_GROUP_X, DESKTOP_ACCESSORIES_Y, DESKTOP_GROUP_W,
              DESKTOP_ACCESSORIES_H, (const char *)0, 0u, UI_CLIENT_WHITE);
    ui_window(DESKTOP_GROUP_X, DESKTOP_GAMES_Y, DESKTOP_GROUP_W,
              DESKTOP_GAMES_H, (const char *)0, 0u, UI_CLIENT_WHITE);
    draw_group_titles();

    for (i = 0u; i != DESKTOP_ICON_COUNT; ++i) draw_icon(i);
}

static UINT8 hovered_icon(void)
{
    UINT8 i;
    const DesktopIcon *icon;

    for (i = 0u; i != DESKTOP_ICON_COUNT; ++i) {
        icon = &desktop_icons[i];
        if (pointer_hits((UINT8)(icon->center - 16u),
                         (UINT8)((icon->icon_y << 3u) - 4u),
                         32u,
                         (UINT8)(((icon->caption_y - icon->icon_y) << 3u) + 12u))) {
            return i;
        }
    }
    return DESKTOP_NO_ICON;
}

static void change_selection(UINT8 next)
{
    UINT8 previous = selected_icon;

    if (next == previous) return;
    selected_icon = next;
    if (previous != DESKTOP_NO_ICON) {
        render_caption(previous, 0u);
        draw_caption(previous);
    }
    if (next != DESKTOP_NO_ICON) {
        render_caption(next, 1u);
        draw_caption(next);
        if (desktop_icons[next].group != active_group) {
            active_group = desktop_icons[next].group;
            draw_group_titles();
        }
    }
}

void desktop_draw_backdrop(void) BANKED
{
    desktop_focused = 0u;
    draw_program_manager();
}

void desktop_enter(void) BANKED
{
    ui_scene_begin();
    assets_restore_palettes();
    selected_icon = 0u;
    active_group = 0u;
    desktop_focused = 1u;
    render_text();
    draw_program_manager();
    pointer_reset(41u, 40u);
    pointer_show();
    ui_scene_end();
}

AppState desktop_update(const InputState *input) BANKED
{
    UINT8 hover;
    const DesktopIcon *icon;

    pointer_update(input);
    hover = hovered_icon();
    change_selection(hover);

    if (input->pressed & J_SELECT) {
        hover = (selected_icon == DESKTOP_NO_ICON) ? 0u :
            (UINT8)((selected_icon + 1u) % DESKTOP_ICON_COUNT);
        change_selection(hover);
        icon = &desktop_icons[hover];
        pointer_move_to((UINT8)(icon->center + 9u), (UINT8)((icon->icon_y << 3u) + 8u));
        audio_sfx(SFX_MOVE);
    }

    if ((input->pressed & J_A) && hover != DESKTOP_NO_ICON) {
        audio_sfx(SFX_CLICK);
        return desktop_icons[hover].app;
    }
    return APP_DESKTOP;
}
