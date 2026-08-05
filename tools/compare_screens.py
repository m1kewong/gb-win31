#!/usr/bin/env python3
"""Pixel-compare captured 160 x 144 screens with committed golden frames."""

from __future__ import annotations

import argparse
import pathlib

from PIL import Image, ImageChops


SCREENS = (
    "boot", "dos", "splash", "desktop", "paint", "piano", "media",
    "sweeper", "cannon",
)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("actual", type=pathlib.Path)
    parser.add_argument("golden", type=pathlib.Path)
    args = parser.parse_args()

    failures: list[str] = []
    for name in SCREENS:
        actual_path = args.actual / f"{name}.png"
        golden_path = args.golden / f"{name}.png"
        if not actual_path.exists() or not golden_path.exists():
            failures.append(f"{name}: missing actual or golden image")
            continue

        with Image.open(actual_path).convert("RGB") as actual:
            with Image.open(golden_path).convert("RGB") as golden:
                if actual.size != (160, 144) or golden.size != (160, 144):
                    failures.append(f"{name}: expected 160x144 frames")
                    continue
                difference = ImageChops.difference(actual, golden)
                if difference.getbbox() is not None:
                    changed = sum(1 for pixel in difference.getdata() if pixel != (0, 0, 0))
                    failures.append(f"{name}: {changed} pixels differ")
                    difference.save(args.actual / f"diff-{name}.png")

    if failures:
        raise SystemExit("visual regression failed:\n" + "\n".join(failures))
    print("visual regression: all nine 160x144 frames match")


if __name__ == "__main__":
    main()
