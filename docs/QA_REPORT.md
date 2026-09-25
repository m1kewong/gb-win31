# GB Win 3.1 verification report

Date: 2026-09-25

## Build result

The cartridge is `build/gb-win31.gbc`: 64 KiB, title `GB WORKBENCH`, CGB flag
`0xC0`, cartridge type `0x1B` (MBC5 + RAM + battery), 8 KiB SRAM, and valid
header/global checksums. Its SHA-256 is
`2b3949d30f33c74129150bdf1fa712f348d3f8836278a126e36c04bf0ab59713`.

| Region | Used | Free |
| --- | --- | --- |
| ROM bank 0 (fixed) | 10,496 B (64%) | 5,888 B |
| ROM bank 1 (switchable) | 16,272 B (99%) | 112 B |
| ROM bank 2 (switchable) | 2,257 B (14%) | 14,127 B |
| WRAM 0xC000-0xCFFF | 3,525 B (86%) | 571 B |

Autobanking filled bank 1 and opened bank 2 when Solitaire was added; the
cartridge grew from 32 to 64 KiB with no source changes beyond the new files.
Scenes share one 64-byte art scratch buffer to keep WRAM headroom.

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
  bounds guards, loss, win) and the Solitaire suite (deal shape, seed
  determinism, draw one/three with recycle penalties, foundation and tableau
  rules, illegal moves, run moves with flips, scoring, auto-foundation, win,
  autocomplete, and a 300-game random soak asserting card conservation and
  tableau invariants after every action) pass with `-Wall -Wextra -Werror`.
  Both suites also pass under AddressSanitizer and UndefinedBehaviorSanitizer.
  The generated font table matches `assets/system_font.txt`.
- `make verify`: size, title, CGB-only flag, MBC5 type, ROM/RAM size codes and
  both checksums.
- `make smoke-test` (PyBoy 2.7.0), synchronised on the exported `gbw_scene`
  byte rather than on rendered text:
  - boot quick-start to desktop, 8x8 pointer with transparent padding;
  - empty desktop click changes nothing;
  - Paint writes the expected bank-1 pixel bytes;
  - Sweeper opens over the Program Manager, flags (LED 010 -> 009), reveals,
    resets from the smiley, and closes from its system box;
  - Piano selects D (pressed key art) and calls `audio_note`;
  - Media plays, changes track, keeps playing after leaving, and stops;
  - Cannon scores, resets score and lives, and exits;
  - Solitaire deals 1-7 with 24 in stock, turns a card, plays a legal move
    found from live WRAM through the snap controls, deals again from the
    Game menu, and from an injected endgame autocompletes to a win and
    redeals with A;
  - pointer reaches the (152, 136) bottom-right bound.
- `make visual-test`: ten deterministic 160 x 144 frames compare
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
- Button presses are latched in the VBlank interrupt, so a tap shorter than
  a slow scene frame is still delivered exactly once (found while testing
  Solitaire's once-a-second status redraw).

## Remaining release gates

See the backlog in `docs/STOCKTAKE.md`: Solitaire win cascade and options,
arrow pointer, Sweeper board sizes, Paint tools and saves, minimise/restore,
Cannon art, printer, and the real-hardware matrix.
