#include <gb/gb.h>

#include "assets.h"
#include "audio.h"
#include "desktop.h"
#include "desktop_text.h"
#include "ui.h"

typedef struct DesktopIcon {
    UINT8 tile_x;
    UINT8 tile_y;
    UINT8 label_x;
    UINT8 label_y;
    UINT8 tile;
    DesktopTextId label;
    AppState app;
    UINT8 group;
} DesktopIcon;

static const DesktopIcon desktop_icons[] = {
    {3u, 4u, 2u, 7u, TILE_ICON_PAINT, DESKTOP_TEXT_PAINT, APP_PAINT, 0u},
    {9u, 4u, 8u, 7u, TILE_ICON_PIANO, DESKTOP_TEXT_PIANO, APP_PIANO, 0u},
    {15u, 4u, 14u, 7u, TILE_ICON_MEDIA, DESKTOP_TEXT_MEDIA, APP_MEDIA, 0u},
    {5u, 12u, 4u, 14u, TILE_ICON_SWEEPER, DESKTOP_TEXT_SWEEPER, APP_SWEEPER, 1u},
    {13u, 12u, 12u, 14u, TILE_ICON_CANNON, DESKTOP_TEXT_CANNON, APP_CANNON, 1u}
};

#define DESKTOP_ICON_COUNT 5u
#define DESKTOP_ACCESSORIES_Y 2u
#define DESKTOP_GAMES_Y 10u
#define DESKTOP_GROUP_X 1u
#define DESKTOP_GROUP_W 18u

static UINT8 selected_icon;
static UINT8 active_group;

static void draw_centered_text(
    UINT8 x,
    UINT8 y,
    UINT8 width,
    DesktopTextId text,
    UINT8 palette
)
{
    UINT8 text_width = desktop_text_width_tiles(text);
    UINT8 text_x = x;

    if (text_width < width) text_x = (UINT8)(x + ((width - text_width) >> 1u));
    desktop_text_draw(text_x, y, text, palette);
}

static void draw_group_title(
    UINT8 y,
    DesktopTextId title,
    UINT8 active
)
{
    UINT8 palette = active ? PAL_TITLE_ACTIVE : PAL_TITLE_INACTIVE;

    ui_fill((UINT8)(DESKTOP_GROUP_X + 1u), y,
            (UINT8)(DESKTOP_GROUP_W - 2u), 1u, TILE_BLANK, palette);
    ui_set_tile((UINT8)(DESKTOP_GROUP_X + 1u), y, TILE_SYSTEM_BUTTON, palette);
    ui_set_tile((UINT8)(DESKTOP_GROUP_X + DESKTOP_GROUP_W - 3u), y,
                TILE_MIN_BUTTON, palette);
    ui_set_tile((UINT8)(DESKTOP_GROUP_X + DESKTOP_GROUP_W - 2u), y,
                TILE_MAX_BUTTON, palette);
    draw_centered_text(DESKTOP_GROUP_X, y, DESKTOP_GROUP_W, title, palette);
}

static void draw_group_titles(void)
{
    draw_group_title(DESKTOP_ACCESSORIES_Y, DESKTOP_TEXT_ACCESSORIES,
                     (UINT8)(active_group == 0u));
    draw_group_title(DESKTOP_GAMES_Y, DESKTOP_TEXT_GAMES,
                     (UINT8)(active_group == 1u));
}

static void draw_icon(const DesktopIcon *icon, UINT8 selected)
{
    ui_set_tile(icon->tile_x, icon->tile_y, icon->tile, PAL_ICON);
    ui_set_tile((UINT8)(icon->tile_x + 1u), icon->tile_y,
                (UINT8)(icon->tile + 1u), PAL_ICON);
    ui_set_tile(icon->tile_x, (UINT8)(icon->tile_y + 1u),
                (UINT8)(icon->tile + 2u), PAL_ICON);
    ui_set_tile((UINT8)(icon->tile_x + 1u), (UINT8)(icon->tile_y + 1u),
                (UINT8)(icon->tile + 3u), PAL_ICON);
    desktop_text_draw(icon->label_x, icon->label_y, icon->label,
                      selected ? PAL_TITLE_ACTIVE : PAL_WINDOW);
}

static void draw_icons(void)
{
    UINT8 i;
    for (i = 0u; i != DESKTOP_ICON_COUNT; ++i) {
        draw_icon(&desktop_icons[i], (UINT8)(i == selected_icon));
    }
}

static UINT8 hovered_icon(void)
{
    UINT8 i;
    for (i = 0u; i != DESKTOP_ICON_COUNT; ++i) {
        if (pointer_hits((UINT8)(desktop_icons[i].label_x * 8u),
                         (UINT8)(desktop_icons[i].tile_y * 8u),
                         32u, 32u)) return i;
    }
    return 0xffu;
}

void desktop_enter(void)
{
    ui_scene_begin();
    desktop_text_load();
    ui_clear(PAL_DESKTOP);

    ui_window(0u, 0u, 20u, 18u, "", 1u);
    draw_centered_text(0u, 0u, 20u, DESKTOP_TEXT_GB_WORKBENCH,
                       PAL_TITLE_ACTIVE);
    desktop_text_draw(2u, 1u, DESKTOP_TEXT_FILE_OPTIONS_HELP, PAL_WINDOW);

    ui_window(DESKTOP_GROUP_X, DESKTOP_ACCESSORIES_Y,
              DESKTOP_GROUP_W, 8u, "", 1u);
    ui_window(DESKTOP_GROUP_X, DESKTOP_GAMES_Y,
              DESKTOP_GROUP_W, 7u, "", 0u);

    selected_icon = 0u;
    active_group = 0u;
    draw_group_titles();
    draw_icons();
    /* Keep the pointer selected while placing the full arrow below the label. */
    pointer_reset(45u, 62u);
    pointer_show();
    ui_scene_end();
}

AppState desktop_update(const InputState *input)
{
    UINT8 hover;
    UINT8 old_selection = selected_icon;
    UINT8 old_group = active_group;

    pointer_update(input);
    hover = hovered_icon();
    selected_icon = hover;
    if (selected_icon != 0xffu) active_group = desktop_icons[selected_icon].group;

    if (input->pressed & J_SELECT) {
        selected_icon = (selected_icon == 0xffu) ? 0u :
            (UINT8)((selected_icon + 1u) % DESKTOP_ICON_COUNT);
        active_group = desktop_icons[selected_icon].group;
        pointer_move_to((UINT8)(desktop_icons[selected_icon].tile_x * 8u + 6u),
                        (UINT8)(desktop_icons[selected_icon].tile_y * 8u + 6u));
        audio_sfx(SFX_MOVE);
    }

    if (old_group != active_group) draw_group_titles();
    if (old_selection != selected_icon) draw_icons();

    if ((input->pressed & J_A) && hover != 0xffu) {
        audio_sfx(SFX_CLICK);
        return desktop_icons[hover].app;
    }
    return APP_DESKTOP;
}
