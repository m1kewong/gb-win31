# GB Win 3.1 clean-room rebuild plan

## Product definition

This project is a native 160 x 144 Game Boy Color desktop simulation inspired by
the interaction language of Windows 3.1. It is not an emulator, a Windows port,
or a copy of the commercial GBS Windows ROM.

Target parity is the *depth of the illusion*: a detailed boot sequence, a
pointer-driven Program Manager, overlapping application windows, five useful
mini-apps, sound, and reliable behavior on real hardware. All code, graphics,
copy, and music in this repository must be clean-room work.

## Audit of commit `5676292`

The initial repository cannot be repaired by polishing assets alone:

- A clean GBDK build fails because source and headers describe different tile
  layouts and reference missing data and functions.
- Application clicks return `*_INIT` states that the main state machine never
  handles.
- UI assets are loaded at one VRAM range and drawn from another.
- Palette index 8 is used even though the CGB exposes background palettes 0-7.
- The checked-in draft ROM was produced from different source and is therefore
  not a reproducible release artifact.
- Minesweeper has an out-of-bounds `board[c][c]` read and recursive flood fill.
- Paint changes whole 8 x 8 tiles instead of individual pixels.
- The majority of frame, title, scrollbar, and icon graphics are placeholders.
- The Makefile's clean rule is Windows-only, while Cloud Build expects a
  different output name and directory.

Captured evidence is kept under `docs/baseline/`.

## Interaction contract

- D-pad: move the visible pointer; apps may also use it for focused controls.
- A: primary click/action; hold while moving to draw in Paint.
- B: secondary action; flag in Sweeper and use Paint's secondary shade.
- Select: cycle focus/tool/color where a pointer target is inconvenient.
- Start: open/close the current app or skip the simulated boot sequence.
- No input handler blocks waiting for button release.

## Architecture

1. A single 60 Hz loop waits for VBlank, samples input once, updates one state,
   renders dirty regions, and advances audio.
2. A small state router owns boot, DOS, Program Manager, and each application.
   State entry is explicit; there are no orphan `INIT` enum values.
3. Bank 0 background tiles contain the compact font, window chrome, icons, and
   application UI. Bank 1 is reserved for mutable Paint canvas tiles.
4. Every palette and tile range is declared in one manifest with compile-time
   bounds checks.
5. The pointer is a sprite. Windows and apps use the background map plus CGB
   attributes, with redraws batched at state changes or limited to dirty tiles.
6. Game logic that does not need hardware registers remains host-testable.

## Implementation status - 2026-08-04

| Slice | Status | Evidence | Remaining work |
| --- | --- | --- | --- |
| 0 | Complete | Reproducible GBDK 4.5.0 build, CGB-only header/checksum verifier, host tests | Add a tagged release workflow |
| 1 | Complete design pass | BIOS, DOS, splash, dual-group Program Manager, packed desktop font, original icons, pointer, five launchable apps, native golden frames | Functional group minimize/maximize controls |
| 2 | Playable first pass | Safe deterministic Sweeper model and true-pixel persistent Paint canvas | Difficulty chooser, Paint save/load, extra palettes/tools |
| 3 | Playable first pass | Piano tones, four original Media tracks with background playback, complete Cannon score/lives loop | Tempo/rhythm controls and a real minimize/overlap window manager |
| 4 | Not started | Roadmap below | Printer, SRAM, SameBoy/mGBA and physical-hardware matrix |

The nine golden screens lock this implementation against accidental visual
regression. They are not a claim that clean-room reference parity is finished.

## Delivery slices and exit gates

### Slice 0 - reproducible foundation

- Pin GBDK 4.5.0 and emit a CGB-only ROM.
- Linux/macOS-compatible `make`, `make clean`, `make test`, and ROM checks.
- Zero missing symbols and no stale ROM/map files tracked as source.
- Correct ROM title, CGB header byte, size, and checksums.

### Slice 1 - boot and Program Manager

- Authored BIOS system table, fake DOS driver load, `CD`/`WIN` sequence, and
  splash transition.
- Cyan desktop, beveled Program Manager/group windows, menus, five original
  icons, active/inactive title styles, and visible pointer.
- Packed 3 x 5 desktop lettering with independent 32-pixel caption cells; no
  caption collision or frame overwrite.
- Pointer hit-testing plus Select focus fallback; all five apps can open/close.
- Golden 160 x 144 screenshots for BIOS, DOS, and desktop.

### Slice 2 - core apps

- Sweeper: first-click safety, deterministic tests, flags, iterative flood
  reveal, win/loss, restart, and three difficulty layouts.
- Paint: true pixel drawing, two assigned shades, multiple palettes, clear/save
  affordances, dirty-tile uploads, and continuous draw while moving.
- Golden screens and scripted input journeys for both apps.

### Slice 3 - playful apps and multitasking illusion

- Piano: playable keys, tone, tempo, and rhythm controls.
- Media: four original tracks, transport controls, and music that continues
  after minimizing.
- Cannon: complete score/lives loop with pointer aiming and restart.

### Slice 4 - hardware and release polish

- Optional Game Boy Printer path, with cancellation and disconnected-device
  behavior.
- Optional SRAM-backed Paint/settings persistence with versioned save data.
- SameBoy and mGBA scripted checks, plus a real GBC/GBA/flashcart smoke matrix.
- Performance/VRAM/ROM budget report and release ROM checksum.

## Definition of pixel-perfect

- Golden images are exact 160 x 144 emulator frame buffers, never photographs
  or upscaled desktop mockups.
- UI geometry, palette indices, tile art, focus, and pointer positions are
  deterministic and reviewed at 1x and nearest-neighbor zoom.
- Visual similarity is evaluated against documented Windows 3.1 conventions;
  commercial GBS Windows photos are behavioral references only.

## Reference material

- Selected native translation and visual comparison:
  `docs/design/DESKTOP_DESIGN.md` and `design-qa.md`
- Authentic Windows 3.1 Program Manager reference (CC0):
  https://commons.wikimedia.org/wiki/File:Program_Manager_de_Windows_3.1.png

- Windows Central overview:
  https://www.windowscentral.com/software-apps/windows-3-1-is-now-available-for-the-game-boy-color-kind-of
- Hands-on visual reference:
  https://retrododo.com/gbs-windows-review/
- GBDK 4.5.0 documentation: https://gbdk.org/docs/api/
- Game Boy technical reference: https://gbdev.io/pandocs/
