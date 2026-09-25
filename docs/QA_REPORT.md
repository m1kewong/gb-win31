# GB Win 3.1 verification report

Date: 2026-09-25

## Build result

The cartridge is `build/gb-win31.gbc`: 32 KiB, title `GB WORKBENCH`, CGB flag
`0xC0`, cartridge type `0x1B` (MBC5 + RAM + battery), 8 KiB SRAM, and valid
header/global checksums. Its SHA-256 is
`521a6dd870172a01e6e3dfd7da061ede1d411681e388ef65dc7bed82a8b1e6b3`.

| Region | Used | Free |
| --- | --- | --- |
| ROM bank 0 (fixed) | 7,588 B (46%) | 8,796 B |
| ROM bank 1 (switchable) | 14,026 B (86%) | 2,358 B |
| WRAM 0xC000-0xCFFF | 3,384 B (83%) | 712 B |

Before this pass the whole program lived in bank 0 at 90% usage. Scenes and
asset builders are now auto-banked; engine code (main loop, input, UI, text,
audio, game models) stays in bank 0. Further code spills into new banks
automatically.

The previous build (`1d6fb920...`, recorded 2026-08-04) was reproduced
byte-for-byte with GBDK 4.5.0 before any change.

## Automated checks

- GBDK 4.5.0 compiles all ROM sources without diagnostics.
- `make test`: the host Sweeper suite (initialisation, every first-click
  position, deterministic mines, adjacency, flags, iterative flood fill,
  bounds guards, loss, win) passes with `-Wall -Wextra -Werror`, and the
  generated font table matches `assets/system_font.txt`.
- `make verify`: size, title, CGB-only flag, MBC5 type, ROM/RAM size codes and
  both checksums.
- `make smoke-test` (PyBoy 2.7.0), synchronised on the exported `gbw_scene`
  byte rather than on rendered text:
  - boot quick-start to desktop; two-sprite arrow pointer with a black tip at
    the hotspot, transparent padding, and the tail sprite locked 8px under the
    head;
  - empty desktop click changes nothing;
  - Paint writes the expected bank-1 pixel bytes;
  - Sweeper opens over the Program Manager, flags (LED 010 -> 009), reveals,
    resets from the smiley, and closes from its system box;
  - Piano selects D (pressed key art) and calls `audio_note`;
  - Media plays, changes track, keeps playing after leaving, and stops;
  - Cannon scores, resets score and lives, and exits;
  - pointer reaches the (152, 136) bottom-right bound.
- `make visual-test`: nine deterministic 160 x 144 frames compare
  pixel-for-pixel with `tests/golden/`. Three consecutive captures were
  identical.

## Manual review gates passed

- Every frame was reviewed at 1x and 3x nearest-neighbour zoom against the
  CC0 Windows 3.1 Program Manager reference and GBS Windows review photos.
- Mixed-case text is legible at native resolution; no status line is clipped.
- Windowed apps (Sweeper, Piano, Media) show the Program Manager behind them
  with inactive title bars; maximised apps (Paint, Cannon) fill the screen.
- Labels are drawn from a per-scene tile pool that is sized at scene entry;
  repeated Cannon rounds reuse pre-allocated labels.

## Remaining release gates

See the backlog in `docs/STOCKTAKE.md`: Solitaire, Sweeper board
sizes, Paint tools and saves, minimise/restore, Cannon art, printer, and the
real-hardware matrix.
