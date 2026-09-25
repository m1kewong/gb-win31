#!/usr/bin/env python3
"""Tile captured 160x144 frames into the README montage (2x nearest-neighbour)."""

from __future__ import annotations

import argparse
import pathlib

from PIL import Image

ORDER = ["boot", "dos", "splash", "desktop", "paint",
         "piano", "media", "sweeper", "cannon", "solitaire"]
SCALE = 2
GAP = 8
COLUMNS = 5


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("screens", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    args = parser.parse_args()

    frames = [args.screens / f"{name}.png" for name in ORDER]
    frames = [path for path in frames if path.exists()]
    rows = (len(frames) + COLUMNS - 1) // COLUMNS
    width = COLUMNS * 160 * SCALE + (COLUMNS - 1) * GAP
    height = rows * 144 * SCALE + (rows - 1) * GAP
    montage = Image.new("RGB", (width, height), (32, 32, 32))
    for index, path in enumerate(frames):
        frame = Image.open(path).convert("RGB").resize((160 * SCALE, 144 * SCALE), Image.NEAREST)
        x = (index % COLUMNS) * (160 * SCALE + GAP)
        y = (index // COLUMNS) * (144 * SCALE + GAP)
        montage.paste(frame, (x, y))
    montage.save(args.output, optimize=True)
    print(f"wrote {args.output} from {len(frames)} frames")


if __name__ == "__main__":
    main()
