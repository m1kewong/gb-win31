#!/usr/bin/env python3
"""Drive a deterministic smoke journey and save exact 160 x 144 frames."""

from __future__ import annotations

import argparse
import pathlib

from pyboy import PyBoy

LCDC_REG = 0xFF40
LCD_ENABLE = 0x80
BG_MAP_SELECT = 0x08
TILE_ICON_PAINT = 96

DESKTOP_PAINT_TILES = (
    (3, 4, TILE_ICON_PAINT),
    (4, 4, TILE_ICON_PAINT + 1),
    (3, 5, TILE_ICON_PAINT + 2),
    (4, 5, TILE_ICON_PAINT + 3),
)


def tick(pyboy: PyBoy, frames: int = 2) -> None:
    for _ in range(frames):
        pyboy.tick()


def tap(pyboy: PyBoy, button: str) -> None:
    pyboy.button_press(button)
    tick(pyboy, 1)
    pyboy.button_release(button)
    tick(pyboy, 2)


def hold(pyboy: PyBoy, buttons: tuple[str, ...], frames: int) -> None:
    for button in buttons:
        pyboy.button_press(button)
    tick(pyboy, frames)
    for button in buttons:
        pyboy.button_release(button)
    tick(pyboy, 3)


def save(pyboy: PyBoy, output: pathlib.Path, name: str) -> None:
    image = pyboy.screen.image
    if image.size != (160, 144):
        raise RuntimeError(f"unexpected frame size {image.size}")
    image.save(output / f"{name}.png")


def font_tile(character: str) -> int:
    value = ord(character.upper())
    if value < 32 or value > 95:
        value = ord("?")
    return value - 32


def text_matches(pyboy: PyBoy, x: int, y: int, value: str) -> bool:
    map_base = 0x9C00 if pyboy.memory[LCDC_REG] & BG_MAP_SELECT else 0x9800
    return all(
        pyboy.memory[map_base + y * 32 + x + offset] == font_tile(character)
        for offset, character in enumerate(value)
    )


def wait_text(
    pyboy: PyBoy,
    x: int,
    y: int,
    value: str,
    timeout: int = 240,
) -> None:
    for _ in range(timeout + 1):
        if pyboy.memory[LCDC_REG] & LCD_ENABLE and text_matches(pyboy, x, y, value):
            tick(pyboy, 3)
            return
        tick(pyboy)
    raise RuntimeError(f"timed out waiting for {value!r} at {x},{y}")


def wait_desktop(pyboy: PyBoy, timeout: int = 240) -> None:
    for _ in range(timeout + 1):
        map_base = 0x9C00 if pyboy.memory[LCDC_REG] & BG_MAP_SELECT else 0x9800
        if (
            pyboy.memory[LCDC_REG] & LCD_ENABLE
            and all(
                pyboy.memory[map_base + y * 32 + x] == tile
                for x, y, tile in DESKTOP_PAINT_TILES
            )
        ):
            tick(pyboy, 3)
            return
        tick(pyboy)
    map_base = 0x9C00 if pyboy.memory[LCDC_REG] & BG_MAP_SELECT else 0x9800
    actual = [
        (x, y, pyboy.memory[map_base + y * 32 + x])
        for x, y, _ in DESKTOP_PAINT_TILES
    ]
    raise RuntimeError(f"timed out waiting for desktop Paint icon; tiles={actual}")


def launch_icon(pyboy: PyBoy, index: int, title: tuple[int, int, str]) -> None:
    for _ in range(index):
        tap(pyboy, "select")
    tap(pyboy, "a")
    wait_text(pyboy, *title)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("rom", type=pathlib.Path)
    parser.add_argument("--out", type=pathlib.Path, default=pathlib.Path("build/screens"))
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)

    pyboy = PyBoy(str(args.rom), window="null", sound_emulated=False)
    pyboy.set_emulation_speed(0)
    try:
        # PyBoy renders its own boot animation before cartridge code starts.
        tick(pyboy, 180)
        save(pyboy, args.out, "boot")
        tap(pyboy, "a")
        wait_text(pyboy, 0, 0, "GB-DOS VERSION 3.10")
        for _ in range(5):
            tap(pyboy, "a")
            tick(pyboy, 8)
        save(pyboy, args.out, "dos")
        tap(pyboy, "a")
        wait_text(pyboy, 4, 3, "GBWORKBENCH")
        save(pyboy, args.out, "splash")
        wait_desktop(pyboy)
        save(pyboy, args.out, "desktop")

        names = ["paint", "piano", "media", "sweeper", "cannon"]
        titles = [
            (2, 0, "GB PAINT"),
            (3, 1, "PIANO"),
            (3, 1, "MEDIA PLAYER"),
            (2, 0, "GB SWEEPER"),
            (3, 0, "CANNON"),
        ]
        for index, (name, title) in enumerate(zip(names, titles)):
            launch_icon(pyboy, index, title)
            if name == "paint":
                hold(pyboy, ("a", "right"), 14)
                hold(pyboy, ("a", "down"), 10)
            elif name == "piano":
                tap(pyboy, "right")
                tap(pyboy, "a")
            elif name == "media":
                tap(pyboy, "a")
                tick(pyboy, 18)
            elif name == "sweeper":
                tap(pyboy, "b")
                tap(pyboy, "select")
                tap(pyboy, "a")
                tick(pyboy, 8)
            else:
                hold(pyboy, ("left",), 8)
                tap(pyboy, "a")
                tick(pyboy, 20)
            save(pyboy, args.out, name)
            tap(pyboy, "start")
            wait_desktop(pyboy)
    finally:
        pyboy.stop(save=False)

    print(f"captured boot, DOS, splash, desktop, and five apps in {args.out}")


if __name__ == "__main__":
    main()
