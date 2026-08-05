#!/usr/bin/env python3
"""Deterministic end-to-end interaction smoke test for the GBC ROM."""

from __future__ import annotations

import argparse
import pathlib
import re
import sys
from collections.abc import Iterable

try:
    from pyboy import PyBoy
except ImportError as error:  # pragma: no cover - exercised only without test deps
    raise SystemExit(
        "PyBoy is required; install requirements-test.txt and set PYBOY_PYTHON"
    ) from error


SCREEN_WIDTH_TILES = 20
SCREEN_HEIGHT_TILES = 18
LCDC_REG = 0xFF40
VBK_REG = 0xFF4F
OAM_BASE = 0xFE00
LCD_ENABLE = 0x80
BG_MAP_SELECT = 0x08
BG_DATA_8000 = 0x10
OBJ_SIZE_8X16 = 0x04

TILE_BLANK = 64
TILE_MS_HIDDEN = 78
TILE_MS_FLAG = 79
TILE_ICON_PAINT = 96
TILE_POINTER_SPRITE = 120

DESKTOP_PAINT_TILES = (
    (3, 4, TILE_ICON_PAINT),
    (4, 4, TILE_ICON_PAINT + 1),
    (3, 5, TILE_ICON_PAINT + 2),
    (4, 5, TILE_ICON_PAINT + 3),
)


class SmokeFailure(AssertionError):
    """Raised with compact emulator state when an interaction fails."""


def _count_hook(counter: dict[str, int]) -> None:
    counter["count"] += 1


def parse_noi_symbols(path: pathlib.Path) -> dict[str, int]:
    symbols: dict[str, int] = {}
    pattern = re.compile(r"^DEF\s+(\S+)\s+0x([0-9A-Fa-f]+)$")
    for line in path.read_text(encoding="utf-8").splitlines():
        match = pattern.match(line.strip())
        if match:
            symbols[match.group(1)] = int(match.group(2), 16)
    return symbols


class RomSmoke:
    def __init__(self, rom: pathlib.Path, noi: pathlib.Path) -> None:
        self.rom = rom
        self.pyboy = PyBoy(str(rom), window="null", sound_emulated=False)
        self.pyboy.set_emulation_speed(0)
        self.hooks: dict[str, dict[str, int]] = {}
        symbols = parse_noi_symbols(noi)
        for symbol in (
            "_audio_note",
            "_audio_music_set",
            "_minesweeper_model_reveal",
            "_minesweeper_model_toggle_flag",
        ):
            if symbol not in symbols:
                raise SmokeFailure(f"missing linker symbol {symbol} in {noi}")
            address = symbols[symbol]
            bank = 0 if address < 0x4000 else 1
            counter = {"count": 0}
            self.pyboy.hook_register(bank, address, _count_hook, counter)
            self.hooks[symbol] = counter

    @property
    def frame(self) -> int:
        return self.pyboy.frame_count

    def stop(self) -> None:
        self.pyboy.stop(save=False)

    def tick(self, frames: int = 1) -> None:
        for _ in range(frames):
            self.pyboy.tick()

    def press(self, buttons: str | Iterable[str], frames: int = 1, settle: int = 2) -> None:
        if isinstance(buttons, str):
            names = (buttons,)
        else:
            names = tuple(buttons)
        for name in names:
            self.pyboy.button_press(name)
        self.tick(frames)
        for name in names:
            self.pyboy.button_release(name)
        self.tick(settle)

    def hook_count(self, symbol: str) -> int:
        return self.hooks[symbol]["count"]

    def pointer_position(self) -> tuple[int, int]:
        sprite_y = self.pyboy.memory[OAM_BASE]
        sprite_x = self.pyboy.memory[OAM_BASE + 1]
        return sprite_x - 8, sprite_y - 16

    def move_pointer_into(
        self,
        bounds: tuple[int, int, int, int],
        buttons: Iterable[str],
        timeout: int = 600,
    ) -> None:
        x0, y0, x1, y1 = bounds
        names = tuple(buttons)
        for name in names:
            self.pyboy.button_press(name)
        try:
            for _ in range(timeout):
                self.tick()
                x, y = self.pointer_position()
                if x0 <= x < x1 and y0 <= y < y1:
                    return
        finally:
            for name in names:
                self.pyboy.button_release(name)
            self.tick(2)
        raise SmokeFailure(
            f"frame {self.frame}: pointer did not enter {bounds}; "
            f"position={self.pointer_position()}"
        )

    def bg_map_base(self) -> int:
        return 0x9C00 if self.pyboy.memory[LCDC_REG] & BG_MAP_SELECT else 0x9800

    def bg_tile(self, x: int, y: int) -> int:
        if not (0 <= x < SCREEN_WIDTH_TILES and 0 <= y < SCREEN_HEIGHT_TILES):
            raise ValueError(f"tile coordinate out of bounds: {x},{y}")
        return self.pyboy.memory[self.bg_map_base() + y * 32 + x]

    @staticmethod
    def font_tile(character: str) -> int:
        value = ord(character.upper())
        if value < 32 or value > 95:
            value = ord("?")
        return value - 32

    def text_matches(self, x: int, y: int, text: str) -> bool:
        if x + len(text) > SCREEN_WIDTH_TILES:
            return False
        return all(self.bg_tile(x + offset, y) == self.font_tile(character)
                   for offset, character in enumerate(text))

    def actual_tiles(self, x: int, y: int, width: int) -> list[int]:
        return [self.bg_tile(x + offset, y) for offset in range(width)]

    def wait_text(self, x: int, y: int, text: str, timeout: int = 180) -> None:
        for _ in range(timeout + 1):
            if (self.pyboy.memory[LCDC_REG] & LCD_ENABLE) and self.text_matches(x, y, text):
                return
            self.tick()
        raise SmokeFailure(
            f"frame {self.frame}: expected text {text!r} at {x},{y}; "
            f"tiles={self.actual_tiles(x, y, len(text))}"
        )

    def assert_text(self, x: int, y: int, text: str) -> None:
        if not self.text_matches(x, y, text):
            raise SmokeFailure(
                f"frame {self.frame}: expected text {text!r} at {x},{y}; "
                f"tiles={self.actual_tiles(x, y, len(text))}"
            )

    def wait_tile_not_in(
        self,
        x: int,
        y: int,
        excluded: set[int],
        timeout: int = 180,
    ) -> int:
        for _ in range(timeout + 1):
            value = self.bg_tile(x, y)
            if value not in excluded:
                return value
            self.tick()
        raise SmokeFailure(
            f"frame {self.frame}: tile {x},{y} remained in {sorted(excluded)}"
        )

    def wait_tile(self, x: int, y: int, expected: int, timeout: int = 180) -> None:
        for _ in range(timeout + 1):
            if self.bg_tile(x, y) == expected:
                return
            self.tick()
        raise SmokeFailure(
            f"frame {self.frame}: tile {x},{y} was {self.bg_tile(x, y)}, "
            f"expected {expected}"
        )

    def visible_map(self) -> bytes:
        base = self.bg_map_base()
        return bytes(
            self.pyboy.memory[base + y * 32 + x]
            for y in range(SCREEN_HEIGHT_TILES)
            for x in range(SCREEN_WIDTH_TILES)
        )

    def vram_bytes(self, bank: int, address: int, length: int) -> bytes:
        previous_bank = self.pyboy.memory[VBK_REG] & 1
        self.pyboy.memory[VBK_REG] = bank & 1
        try:
            return bytes(self.pyboy.memory[address:address + length])
        finally:
            self.pyboy.memory[VBK_REG] = previous_bank

    def bg_pattern_address(self, tile: int) -> int:
        if self.pyboy.memory[LCDC_REG] & BG_DATA_8000:
            return 0x8000 + tile * 16
        signed_tile = tile if tile < 128 else tile - 256
        return 0x9000 + signed_tile * 16

    def wait_desktop(self, timeout: int = 180) -> None:
        for _ in range(timeout + 1):
            if (
                self.pyboy.memory[LCDC_REG] & LCD_ENABLE
                and all(
                    self.bg_tile(x, y) == tile
                    for x, y, tile in DESKTOP_PAINT_TILES
                )
            ):
                return
            self.tick()
        actual = [(x, y, self.bg_tile(x, y)) for x, y, _ in DESKTOP_PAINT_TILES]
        raise SmokeFailure(
            f"frame {self.frame}: desktop Paint icon was not ready; tiles={actual}"
        )

    def assert_pointer_sprite(self) -> None:
        if self.pyboy.memory[LCDC_REG] & OBJ_SIZE_8X16:
            raise SmokeFailure(f"frame {self.frame}: pointer is not in 8x8 mode")

        pointer_data = self.vram_bytes(
            0,
            0x8000 + TILE_POINTER_SPRITE * 16,
            16,
        )
        if pointer_data[-2:] != bytes(2) or any(byte & 1 for byte in pointer_data):
            raise SmokeFailure(
                f"frame {self.frame}: triangle has no transparent edge padding"
            )

    def launch_icon(self, index: int, title: tuple[int, int, str]) -> None:
        self.wait_desktop()
        for _ in range(index):
            self.press("select", settle=4)
        self.press("a")
        self.wait_text(*title)
        # Scene construction can span frames; let the launch-button release
        # reach input_update before reusing A inside the application.
        self.tick(3)


def run_smoke(smoke: RomSmoke) -> None:
    # Boot and quick-start into a deterministic desktop.
    smoke.wait_text(1, 0, "GB WORKBENCH BIOS", timeout=240)
    smoke.press("start")
    smoke.wait_desktop()
    smoke.assert_pointer_sprite()
    print("ok boot -> desktop")

    # A click away from every icon must not launch the stale keyboard selection.
    smoke.press("left", frames=32)
    before_empty_click = smoke.visible_map()
    smoke.press("a", settle=40)
    smoke.wait_desktop()
    if smoke.visible_map() != before_empty_click:
        raise SmokeFailure(f"frame {smoke.frame}: empty desktop click changed the tile map")
    smoke.press("right", frames=32)
    print("ok desktop empty click")

    # Paint: draw at the initial pointer position and verify the bank-1 tile bytes.
    smoke.launch_icon(0, (2, 0, "GB PAINT"))
    paint_tile = 5 * 14 + 7
    paint_address = smoke.bg_pattern_address(paint_tile)
    before_paint = smoke.vram_bytes(1, paint_address, 2)
    if before_paint != b"\xff\xff":
        raise SmokeFailure(
            f"frame {smoke.frame}: fresh Paint pixel was not blank: {before_paint.hex()}"
        )
    smoke.press("a")
    after_paint = smoke.vram_bytes(1, paint_address, 2)
    if after_paint != b"\x7f\x7f":
        raise SmokeFailure(
            f"frame {smoke.frame}: Paint pixel write was {after_paint.hex()}, expected 7f7f"
        )
    smoke.press("start")
    smoke.wait_desktop()
    print("ok Paint draw -> desktop")

    # Sweeper: flag, reveal another cell, reset from Game, then exit with Start.
    smoke.launch_icon(3, (2, 0, "GB SWEEPER"))
    focus_x, focus_y = 10, 9
    if smoke.bg_tile(focus_x, focus_y) != TILE_MS_HIDDEN:
        raise SmokeFailure(f"frame {smoke.frame}: initial Sweeper focus was not hidden")

    toggle_before = smoke.hook_count("_minesweeper_model_toggle_flag")
    smoke.press("b")
    if smoke.hook_count("_minesweeper_model_toggle_flag") != toggle_before + 1:
        raise SmokeFailure(f"frame {smoke.frame}: Sweeper flag action was not dispatched")
    smoke.wait_tile(focus_x, focus_y, TILE_MS_FLAG)
    smoke.wait_text(10, 3, "FLAGS:01")

    smoke.press("select")
    reveal_before = smoke.hook_count("_minesweeper_model_reveal")
    smoke.press("a")
    if smoke.hook_count("_minesweeper_model_reveal") != reveal_before + 1:
        raise SmokeFailure(f"frame {smoke.frame}: Sweeper reveal action was not dispatched")
    smoke.wait_tile_not_in(11, 9, {TILE_MS_HIDDEN, TILE_MS_FLAG})

    smoke.move_pointer_into((8, 8, 40, 16), ("left", "up"))
    smoke.press("a")
    smoke.wait_text(10, 3, "FLAGS:00")
    if smoke.bg_tile(focus_x, focus_y) != TILE_MS_HIDDEN:
        raise SmokeFailure(f"frame {smoke.frame}: Game-menu reset did not restore the board")
    smoke.press("start")
    smoke.wait_desktop()
    print("ok Sweeper flag/reveal/reset/exit")

    # Piano: move to D, play it, and prove audio_note was called.
    smoke.launch_icon(1, (3, 1, "PIANO"))
    smoke.press("right")
    smoke.wait_text(12, 5, "D")
    notes_before = smoke.hook_count("_audio_note")
    smoke.press("a")
    if smoke.hook_count("_audio_note") != notes_before + 1:
        raise SmokeFailure(f"frame {smoke.frame}: Piano A did not call audio_note")
    smoke.press("start")
    smoke.wait_desktop()
    print("ok Piano play -> desktop")

    # Media: play, advance track while playing, leave, and verify persisted playback.
    smoke.launch_icon(2, (3, 1, "MEDIA PLAYER"))
    music_before = smoke.hook_count("_audio_music_set")
    smoke.press("a")
    smoke.wait_text(4, 5, "NOW PLAYING")
    if smoke.hook_count("_audio_music_set") != music_before + 1:
        raise SmokeFailure(f"frame {smoke.frame}: Media play did not start music")
    smoke.press("down")
    smoke.wait_text(4, 6, "MODEM MOON")
    if smoke.hook_count("_audio_music_set") != music_before + 2:
        raise SmokeFailure(f"frame {smoke.frame}: Media track change did not update music")
    smoke.press("start")
    smoke.wait_desktop()
    smoke.launch_icon(2, (3, 1, "MEDIA PLAYER"))
    smoke.assert_text(4, 5, "NOW PLAYING")
    smoke.assert_text(4, 6, "MODEM MOON")
    smoke.press("b")
    smoke.wait_text(4, 5, "PLAYER READY")
    smoke.press("start")
    smoke.wait_desktop()
    print("ok Media play/track/persistence/exit")

    # Cannon: align with the middle target, fire to score, reset, and exit.
    smoke.launch_icon(4, (3, 0, "CANNON"))
    smoke.assert_text(8, 1, "00")
    smoke.press("left", frames=8)
    smoke.press("a")
    smoke.wait_text(8, 1, "01", timeout=120)
    smoke.press("b")
    smoke.wait_text(8, 1, "00")
    smoke.assert_text(17, 1, "3")
    smoke.press("start")
    smoke.wait_desktop()
    print("ok Cannon fire/reset/exit")

    smoke.press(("right", "down"), frames=200)
    if smoke.pointer_position() != (152, 136):
        raise SmokeFailure(
            f"frame {smoke.frame}: 8x8 pointer bounds were {smoke.pointer_position()}"
        )
    print("ok full cursor bounds")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("rom", type=pathlib.Path)
    parser.add_argument("--noi", type=pathlib.Path)
    parser.add_argument("--failure-screenshot", type=pathlib.Path)
    args = parser.parse_args()

    rom = args.rom.resolve()
    noi = (args.noi or rom.with_suffix(".noi")).resolve()
    failure_screenshot = args.failure_screenshot or rom.parent / "smoke-failure.png"
    if not rom.is_file():
        raise SystemExit(f"ROM not found: {rom}")
    if not noi.is_file():
        raise SystemExit(f"linker symbol file not found: {noi}")

    smoke: RomSmoke | None = None
    try:
        smoke = RomSmoke(rom, noi)
        run_smoke(smoke)
        failure_screenshot.unlink(missing_ok=True)
        print(f"interaction smoke passed in {smoke.frame} frames")
    except Exception:
        if smoke is not None:
            failure_screenshot.parent.mkdir(parents=True, exist_ok=True)
            smoke.pyboy.screen.image.save(failure_screenshot)
            print(f"failure frame saved to {failure_screenshot}", file=sys.stderr)
        raise
    finally:
        if smoke is not None:
            smoke.stop()


if __name__ == "__main__":
    main()
