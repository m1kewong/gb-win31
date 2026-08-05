#!/usr/bin/env python3
"""Fail fast when a produced ROM is not the intended CGB artifact."""

from __future__ import annotations

import pathlib
import sys


def fail(message: str) -> None:
    raise SystemExit(f"ROM verification failed: {message}")


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("usage: verify_rom.py ROM.gbc")

    path = pathlib.Path(sys.argv[1])
    rom = path.read_bytes()
    if len(rom) < 0x150 or len(rom) % 0x4000:
        fail(f"unexpected length {len(rom)} bytes")

    title = rom[0x134:0x143].split(b"\0", 1)[0].decode("ascii", "replace")
    if title != "GB WORKBENCH":
        fail(f"unexpected title {title!r}")
    if rom[0x143] != 0xC0:
        fail(f"CGB flag is 0x{rom[0x143]:02x}, expected CGB-only 0xc0")

    header_checksum = 0
    for value in rom[0x134:0x14D]:
        header_checksum = (header_checksum - value - 1) & 0xFF
    if header_checksum != rom[0x14D]:
        fail("header checksum mismatch")

    global_checksum = sum(rom[:0x14E]) + sum(rom[0x150:])
    global_checksum &= 0xFFFF
    stored_global_checksum = (rom[0x14E] << 8) | rom[0x14F]
    if global_checksum != stored_global_checksum:
        fail("global checksum mismatch")

    print(
        f"ROM OK: {path} | {len(rom) // 1024} KiB | "
        f"title={title!r} | CGB-only | checksums valid"
    )


if __name__ == "__main__":
    main()
