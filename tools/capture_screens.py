#!/usr/bin/env python3
"""Drive a deterministic journey and save exact 160 x 144 frames."""

from __future__ import annotations

import argparse
import pathlib

import gbw_emu as emu


def save(machine: emu.Emulator, output: pathlib.Path, name: str) -> None:
    image = machine.pyboy.screen.image
    if image.size != (160, 144):
        raise RuntimeError(f"unexpected frame size {image.size}")
    image.save(output / f"{name}.png")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("rom", type=pathlib.Path)
    parser.add_argument("--out", type=pathlib.Path, default=pathlib.Path("build/screens"))
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)

    machine = emu.Emulator(args.rom)
    try:
        # PyBoy renders its own boot animation before cartridge code starts.
        machine.tick(180)
        save(machine, args.out, "boot")
        machine.press("a")
        machine.wait_scene(emu.SCENE_DOS)
        for _ in range(5):
            machine.press("a")
            machine.tick(8)
        save(machine, args.out, "dos")
        machine.press("a")
        machine.wait_until(
            lambda: machine.lcd_on() and machine.bg_tile(2, 4) == emu.TILE_FRAME_TL,
            "splash window",
        )
        machine.tick(3)
        save(machine, args.out, "splash")
        machine.wait_scene(emu.SCENE_DESKTOP)
        save(machine, args.out, "desktop")

        apps = [
            ("paint", emu.SCENE_PAINT),
            ("piano", emu.SCENE_PIANO),
            ("media", emu.SCENE_MEDIA),
            ("sweeper", emu.SCENE_SWEEPER),
            ("cannon", emu.SCENE_CANNON),
        ]
        for index, (name, scene) in enumerate(apps):
            machine.launch_icon(index, scene)
            if name == "paint":
                machine.press(("a", "right"), frames=14, settle=3)
                machine.press(("a", "down"), frames=10, settle=3)
            elif name == "piano":
                machine.press("right")
                machine.press("a")
            elif name == "media":
                machine.press("a")
                machine.tick(18)
            elif name == "sweeper":
                machine.press("b")
                machine.press("select")
                machine.press("a")
                machine.tick(8)
            else:
                machine.press("left", frames=8, settle=3)
                machine.press("a")
                machine.tick(20)
            save(machine, args.out, name)
            machine.press("start")
            machine.wait_scene(emu.SCENE_DESKTOP)
    finally:
        machine.stop()

    print(f"captured boot, DOS, splash, desktop, and five apps in {args.out}")


if __name__ == "__main__":
    main()
