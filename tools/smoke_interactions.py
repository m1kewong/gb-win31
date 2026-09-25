#!/usr/bin/env python3
"""Deterministic end-to-end interaction smoke test for the GBC ROM."""

from __future__ import annotations

import argparse
import pathlib
import sys

import gbw_emu as emu
from gbw_emu import EmuFailure

OBJ_SIZE_8X16 = 0x04

# Sweeper layout and bank-1 art (mirrors src/minesweeper.c).
MS_BOARD_X = 5
MS_BOARD_Y = 8
MS_ART_HIDDEN = 0
MS_ART_FLAG = 1
MS_ART_LED = 13
MS_COUNTER_X = 5
MS_HEADER_Y = 4
MS_FACE_CENTER = (80, 40)
MS_CLOSE_BOX = (30, 10)

# Piano key art: region * 12 + half * 6 + edge * 2 + pressed (src/piano.c).
PIANO_KEY_D_LOWER_LEFT = (4, 9)
PIANO_PRESSED_LOWER_LEFT = 13


def led_tile(digit: int) -> int:
    return MS_ART_LED + digit * 2


def assert_pointer_sprite(machine: emu.Emulator) -> None:
    if machine.pyboy.memory[emu.LCDC_REG] & OBJ_SIZE_8X16:
        raise EmuFailure(f"frame {machine.frame}: pointer is not in 8x8 mode")
    pointer = machine.vram_bytes(0, 0x8000 + emu.TILE_POINTER_SPRITE * 16, 16)
    if pointer[-2:] != bytes(2) or any(byte & 1 for byte in pointer):
        raise EmuFailure(f"frame {machine.frame}: triangle has no transparent edge padding")


def wait_counter(machine: emu.Emulator, value: int) -> None:
    wanted = [led_tile(int(d)) for d in f"{value:03d}"]
    machine.wait_until(
        lambda: [machine.bg_tile(MS_COUNTER_X + i, MS_HEADER_Y) for i in range(3)] == wanted,
        f"mine counter {value:03d}",
    )


def run_smoke(machine: emu.Emulator) -> None:
    for symbol in (
        "_audio_note",
        "_audio_music_set",
        "_minesweeper_model_reveal",
        "_minesweeper_model_toggle_flag",
    ):
        machine.hook(symbol)

    machine.wait_scene(emu.SCENE_BOOT)
    machine.press("start")
    machine.wait_scene(emu.SCENE_DESKTOP)
    assert_pointer_sprite(machine)
    print("ok boot -> desktop")

    # A click away from every icon must not launch the stale keyboard selection.
    machine.press("left", frames=32)
    before_empty_click = machine.visible_map()
    machine.press("a", settle=40)
    if machine.scene() != emu.SCENE_DESKTOP or machine.visible_map() != before_empty_click:
        raise EmuFailure(f"frame {machine.frame}: empty desktop click changed the desktop")
    machine.press("right", frames=32)
    print("ok desktop empty click")

    # Paint: draw at the initial pointer position and verify the bank-1 tile bytes.
    machine.launch_icon(0, emu.SCENE_PAINT)
    paint_address = machine.bg_pattern_address(5 * 14 + 7)
    if machine.vram_bytes(1, paint_address, 2) != b"\xff\xff":
        raise EmuFailure(f"frame {machine.frame}: fresh Paint pixel was not blank")
    machine.press("a")
    after_paint = machine.vram_bytes(1, paint_address, 2)
    if after_paint != b"\x7f\x7f":
        raise EmuFailure(f"frame {machine.frame}: Paint pixel write was {after_paint.hex()}")
    machine.press("start")
    machine.wait_scene(emu.SCENE_DESKTOP)
    print("ok Paint draw -> desktop")

    # Sweeper: a window over the Program Manager; flag, reveal, smiley reset, close box.
    machine.launch_icon(3, emu.SCENE_SWEEPER)
    if machine.bg_tile(0, 0) != emu.TILE_FRAME_TL or machine.bg_tile(3, 1) != emu.TILE_FRAME_TL:
        raise EmuFailure(f"frame {machine.frame}: Sweeper is not drawn over the Program Manager")
    focus = (MS_BOARD_X + 5, MS_BOARD_Y + 4)
    if machine.bg_tile(*focus) != MS_ART_HIDDEN:
        raise EmuFailure(f"frame {machine.frame}: initial Sweeper focus was not hidden")
    wait_counter(machine, 10)

    toggles = machine.hook_count("_minesweeper_model_toggle_flag")
    machine.press("b")
    if machine.hook_count("_minesweeper_model_toggle_flag") != toggles + 1:
        raise EmuFailure(f"frame {machine.frame}: Sweeper flag action was not dispatched")
    machine.wait_tile(*focus, MS_ART_FLAG)
    wait_counter(machine, 9)

    machine.press("select")
    reveals = machine.hook_count("_minesweeper_model_reveal")
    machine.press("a")
    if machine.hook_count("_minesweeper_model_reveal") != reveals + 1:
        raise EmuFailure(f"frame {machine.frame}: Sweeper reveal action was not dispatched")
    machine.wait_until(
        lambda: machine.bg_tile(focus[0] + 1, focus[1]) not in (MS_ART_HIDDEN, MS_ART_FLAG),
        "revealed Sweeper cell",
    )

    machine.move_pointer_to(*MS_FACE_CENTER)
    machine.press("a")
    machine.wait_tile(*focus, MS_ART_HIDDEN)
    wait_counter(machine, 10)

    machine.move_pointer_to(*MS_CLOSE_BOX)
    machine.press("a")
    machine.wait_scene(emu.SCENE_DESKTOP)
    print("ok Sweeper flag/reveal/smiley reset/close box")

    # Piano: move to D, play it, and prove audio_note was called.
    machine.launch_icon(1, emu.SCENE_PIANO)
    machine.press("right")
    machine.wait_byte("_piano_key", 1)
    machine.wait_tile(*PIANO_KEY_D_LOWER_LEFT, PIANO_PRESSED_LOWER_LEFT)
    notes = machine.hook_count("_audio_note")
    machine.press("a")
    if machine.hook_count("_audio_note") != notes + 1:
        raise EmuFailure(f"frame {machine.frame}: Piano A did not call audio_note")
    machine.press("start")
    machine.wait_scene(emu.SCENE_DESKTOP)
    print("ok Piano play -> desktop")

    # Media: play, change track while playing, leave, and verify persisted playback.
    machine.launch_icon(2, emu.SCENE_MEDIA)
    music = machine.hook_count("_audio_music_set")
    machine.press("a")
    machine.wait_byte("_media_playing", 1)
    if machine.hook_count("_audio_music_set") != music + 1:
        raise EmuFailure(f"frame {machine.frame}: Media play did not start music")
    machine.press("down")
    machine.wait_byte("_media_track", 1)
    if machine.hook_count("_audio_music_set") != music + 2:
        raise EmuFailure(f"frame {machine.frame}: Media track change did not update music")
    machine.press("start")
    machine.wait_scene(emu.SCENE_DESKTOP)
    machine.launch_icon(2, emu.SCENE_MEDIA)
    if machine.byte("_media_playing") != 1 or machine.byte("_media_track") != 1:
        raise EmuFailure(f"frame {machine.frame}: Media playback did not persist")
    machine.press("b")
    machine.wait_byte("_media_playing", 0)
    machine.press("start")
    machine.wait_scene(emu.SCENE_DESKTOP)
    print("ok Media play/track/persistence/exit")

    # Cannon: align with the middle target, fire to score, reset, and exit.
    machine.launch_icon(4, emu.SCENE_CANNON)
    if machine.byte("_cannon_score") != 0:
        raise EmuFailure(f"frame {machine.frame}: Cannon did not start at zero")
    machine.press("left", frames=8)
    machine.press("a")
    machine.wait_byte("_cannon_score", 1, timeout=120)
    machine.press("b")
    machine.wait_byte("_cannon_score", 0)
    if machine.byte("_cannon_lives") != 3:
        raise EmuFailure(f"frame {machine.frame}: Cannon reset did not restore lives")
    machine.press("start")
    machine.wait_scene(emu.SCENE_DESKTOP)
    print("ok Cannon fire/reset/exit")

    machine.press(("right", "down"), frames=200)
    if machine.pointer_position() != (152, 136):
        raise EmuFailure(f"frame {machine.frame}: 8x8 pointer bounds were {machine.pointer_position()}")
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

    machine: emu.Emulator | None = None
    try:
        machine = emu.Emulator(rom, noi)
        run_smoke(machine)
        failure_screenshot.unlink(missing_ok=True)
        print(f"interaction smoke passed in {machine.frame} frames")
    except Exception:
        if machine is not None:
            failure_screenshot.parent.mkdir(parents=True, exist_ok=True)
            machine.pyboy.screen.image.save(failure_screenshot)
            print(f"failure frame saved to {failure_screenshot}", file=sys.stderr)
        raise
    finally:
        if machine is not None:
            machine.stop()


if __name__ == "__main__":
    main()
