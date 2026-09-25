"""Shared PyBoy harness for GB Workbench emulator tests."""

from __future__ import annotations

import pathlib
import re
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

# Mirrors include/app.h.
SCENE_BOOT = 0
SCENE_DOS = 1
SCENE_DESKTOP = 2
SCENE_SWEEPER = 3
SCENE_PAINT = 4
SCENE_PIANO = 5
SCENE_MEDIA = 6
SCENE_CANNON = 7
SCENE_SOLITAIRE = 8

# Mirrors include/assets.h.
TILE_FRAME_TL = 66
TILE_POINTER_SPRITE = 120


class EmuFailure(AssertionError):
    """Raised with compact emulator state when an expectation fails."""


def parse_noi_symbols(path: pathlib.Path) -> dict[str, int]:
    symbols: dict[str, int] = {}
    pattern = re.compile(r"^DEF\s+(\S+)\s+0x([0-9A-Fa-f]+)$")
    for line in path.read_text(encoding="utf-8").splitlines():
        match = pattern.match(line.strip())
        if match:
            symbols[match.group(1)] = int(match.group(2), 16)
    return symbols


def _count_hook(counter: dict[str, int]) -> None:
    counter["count"] += 1


class Emulator:
    def __init__(self, rom: pathlib.Path, noi: pathlib.Path | None = None) -> None:
        self.rom = rom
        self.symbols = parse_noi_symbols(noi or rom.with_suffix(".noi"))
        self.pyboy = PyBoy(str(rom), window="null", sound_emulated=False)
        self.pyboy.set_emulation_speed(0)
        self.hooks: dict[str, dict[str, int]] = {}

    # --- lifecycle and input -------------------------------------------------

    @property
    def frame(self) -> int:
        return self.pyboy.frame_count

    def stop(self) -> None:
        self.pyboy.stop(save=False)

    def tick(self, frames: int = 1) -> None:
        for _ in range(frames):
            self.pyboy.tick()

    def press(self, buttons: str | Iterable[str], frames: int = 1, settle: int = 2) -> None:
        names = (buttons,) if isinstance(buttons, str) else tuple(buttons)
        for name in names:
            self.pyboy.button_press(name)
        self.tick(frames)
        for name in names:
            self.pyboy.button_release(name)
        self.tick(settle)

    # --- symbols -------------------------------------------------------------

    def address(self, symbol: str) -> int:
        if symbol not in self.symbols:
            raise EmuFailure(f"missing linker symbol {symbol}")
        return self.symbols[symbol]

    def byte(self, symbol: str) -> int:
        return self.pyboy.memory[self.address(symbol) & 0xFFFF]

    def hook(self, symbol: str) -> None:
        address = self.address(symbol)
        bank = address >> 16
        offset = address & 0xFFFF
        if offset < 0x4000:
            bank = 0
        counter = {"count": 0}
        self.pyboy.hook_register(bank, offset, _count_hook, counter)
        self.hooks[symbol] = counter

    def hook_count(self, symbol: str) -> int:
        return self.hooks[symbol]["count"]

    # --- screen state --------------------------------------------------------

    def lcd_on(self) -> bool:
        return bool(self.pyboy.memory[LCDC_REG] & LCD_ENABLE)

    def scene(self) -> int:
        value = self.byte("_gbw_scene")
        return value & 0x7F if value & 0x80 else -1

    def bg_map_base(self) -> int:
        return 0x9C00 if self.pyboy.memory[LCDC_REG] & BG_MAP_SELECT else 0x9800

    def bg_tile(self, x: int, y: int) -> int:
        if not (0 <= x < SCREEN_WIDTH_TILES and 0 <= y < SCREEN_HEIGHT_TILES):
            raise ValueError(f"tile coordinate out of bounds: {x},{y}")
        return self.pyboy.memory[self.bg_map_base() + y * 32 + x]

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

    def pointer_position(self) -> tuple[int, int]:
        return self.pyboy.memory[OAM_BASE + 1] - 8, self.pyboy.memory[OAM_BASE] - 16

    # --- waits ---------------------------------------------------------------

    def wait_until(self, predicate, description: str, timeout: int = 240) -> None:
        for _ in range(timeout + 1):
            if predicate():
                return
            self.tick()
        raise EmuFailure(f"frame {self.frame}: timed out waiting for {description}")

    def wait_scene(self, scene: int, timeout: int = 240, settle: int = 3) -> None:
        self.wait_until(
            lambda: self.lcd_on() and self.scene() == scene,
            f"scene {scene} (current {self.scene()})",
            timeout,
        )
        self.tick(settle)

    def wait_tile(self, x: int, y: int, expected: int, timeout: int = 180) -> None:
        self.wait_until(
            lambda: self.bg_tile(x, y) == expected,
            f"tile {x},{y} == {expected} (is {self.bg_tile(x, y)})",
            timeout,
        )

    def wait_byte(self, symbol: str, expected: int, timeout: int = 180) -> None:
        self.wait_until(
            lambda: self.byte(symbol) == expected,
            f"{symbol} == {expected} (is {self.byte(symbol)})",
            timeout,
        )

    # --- pointer movement ----------------------------------------------------

    def move_pointer_to(self, x: int, y: int, timeout: int = 400) -> None:
        """Walk the pointer one axis at a time so paths are predictable."""
        for axis, target in ((1, y), (0, x)):
            current = self.pointer_position()[axis]
            if current == target:
                continue
            if axis == 0:
                button = "right" if target > current else "left"
            else:
                button = "down" if target > current else "up"
            self.pyboy.button_press(button)
            try:
                for _ in range(timeout):
                    self.tick()
                    if self.pointer_position()[axis] == target:
                        break
                else:
                    raise EmuFailure(
                        f"frame {self.frame}: pointer stuck at {self.pointer_position()}"
                    )
            finally:
                self.pyboy.button_release(button)
                self.tick(2)
        # OAM is copied at VBlank, so it trails the pointer by a frame; nudge
        # back one pixel at a time after the coarse move.
        for _ in range(8):
            px, py = self.pointer_position()
            if (px, py) == (x, y):
                break
            if py != y:
                self.press("down" if y > py else "up", frames=1, settle=3)
            else:
                self.press("right" if x > px else "left", frames=1, settle=3)
        if self.pointer_position() != (x, y):
            raise EmuFailure(
                f"frame {self.frame}: pointer at {self.pointer_position()}, wanted {(x, y)}"
            )

    def launch_icon(self, index: int, scene: int) -> None:
        self.wait_scene(SCENE_DESKTOP, settle=0)
        for _ in range(index):
            self.press("select", settle=4)
        self.press("a")
        self.wait_scene(scene)
