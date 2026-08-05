# GB Win 3.1 verification report

Date: 2026-08-04

## Rebuild result

The original snapshot at commit `5676292` was not reproducible from its source.
The rebuild replaces the mismatched state, tile, palette, Paint, Sweeper, and
build paths with a clean-room CGB-only implementation.

The generated cartridge is `build/gb-win31.gbc`: 32 KiB, title
`GB WORKBENCH`, CGB flag `0xC0`, and valid header/global checksums.
Its SHA-256 is
`4b446c5a2cba9bc26b1b7b655d28f0e8896cfd8e614417b2f4ff7bf29e8053cb`;
the active 16 KiB ROM bank uses 14,716 bytes (90%), leaving 1,668 bytes.
WRAM uses 2,825 of 4,096 bytes, leaving 1,271 bytes.

## Automated checks

- GBDK 4.5.0 compiles all ROM sources without diagnostics.
- The host Sweeper suite covers initialization, all first-click positions,
  deterministic mine placement, adjacency, flags, iterative flood fill,
  bounds guards, loss, and win.
- The same model suite passes AddressSanitizer and UndefinedBehaviorSanitizer;
  leak detection is disabled because it is unavailable in the sandbox.
- PyBoy 2.7.0 drives the complete boot-to-desktop journey and every app. The
  interaction smoke covers empty-space clicks, pixel drawing, flag/reveal/reset,
  Piano note dispatch, Media persistence, and Cannon score/reset in one
  deterministic journey.
- Nine deterministic 160 x 144 frames compare pixel-for-pixel with
  `tests/golden/`.
- Desktop detection is anchored to the complete 2 x 2 Paint icon rather than
  the old full-tile title text, so the packed desktop font remains testable.
- Cursor regression coverage verifies 8 x 16 sprite mode, transparent tail
  padding, and the `(152, 128)` bottom-right position that keeps it on-screen.
- `tools/verify_rom.py` checks cartridge size alignment, title, CGB-only flag,
  and both checksums.

## Manual review gates passed

- BIOS, fake DOS, splash, Program Manager, and all five applications are
  legible at native resolution.
- The Program Manager matches the selected dual-group design: captions occupy
  independent 32-pixel cells, active and inactive groups are distinct, the
  complete pointer sits below the selected caption, and the compact `W` remains
  readable in `WINDOW` and `SWEEPER`.
- Source and implementation were compared together at the same 160 x 144
  viewport and at 4x nearest-neighbor zoom; `design-qa.md` records a pass.
- Every application can be launched and closed without a blocking input loop.
- Paint changes individual pixels and uploads only affected mutable tiles.
- Sweeper never uses recursive reveal and its first revealed cell is safe.
- Media music continues after returning to the desktop.
- Whole-scene redraws disable the LCD instead of misusing the CGB background
  priority bit, preventing exposed partial redraws.

## Remaining release gates

- Add a true overlapping/minimizing window manager and functional title-bar
  controls.
- Add app-specific pixel-art polish outside the completed Program Manager pass.
- Add Sweeper difficulty selection and Paint tools/persistence.
- Add printer support only after disconnected/cancel behavior is designed.
- Run the documented SameBoy, mGBA, real GBC/GBA, and flashcart smoke matrix.

These remaining gates are why the current build is called a functional first
vertical slice, not finished commercial-reference parity.
