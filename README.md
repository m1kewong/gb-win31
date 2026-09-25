# GB Win 3.1

A clean-room Game Boy Color desktop simulation inspired by classic Windows 3.1
interaction and visual design. The ROM provides a BIOS/DOS boot illusion, a
pointer-driven Program Manager, Paint, Piano, Media Player, Sweeper, Cannon,
Solitaire, sound, and deterministic visual tests. Solitaire is the one
classic Windows 3.1 game that GBS Windows does not include.

The interface uses a mixed-case proportional system font, white client areas,
Windows 3.1 title-bar buttons, and application windows that open over an
inactive Program Manager.

![Current native-resolution journey](docs/screens/current-montage.png)

This is native GBC software, not a Windows emulator or a copy of the commercial
GBS Windows ROM. The comparison with GBS Windows is in
[docs/STOCKTAKE.md](docs/STOCKTAKE.md); the phased roadmap is in
[docs/REBUILD_PLAN.md](docs/REBUILD_PLAN.md); current test evidence is in
[docs/QA_REPORT.md](docs/QA_REPORT.md).

## Build

Install [GBDK 4.5.0](https://github.com/gbdk-2020/gbdk-2020/releases/tag/4.5.0),
then run:

```sh
make GBDK_HOME=/path/to/gbdk
```

The ROM is written to `build/gb-win31.gbc`: a CGB-only MBC5 cartridge with
8 KiB battery SRAM. Sources that declare `#pragma bank 255` are placed in
switchable ROM banks automatically.

```sh
make test      # host unit tests and font-table freshness check
make verify    # cartridge header, type and checksums
make budget    # ROM/WRAM bank usage
```

For exact-frame emulator regression tests, install `requirements-test.txt` in a
virtual environment and run `make smoke-test` and `make visual-test` with
`PYBOY_PYTHON` pointing at it. After an intentional visual change, review the
frames and run `make golden montage`.

The system font is edited as pixel art in `assets/system_font.txt`; run
`make font` to regenerate `src/text_font.c`.

## Controls

- D-pad: move the pointer or focused app control.
- A: click, play, reveal, fire, or draw with Paint's primary shade.
- B: flag in Sweeper, stop in Media, or draw with Paint's secondary shade.
- Select: cycle desktop focus, Piano tone, Media controls, or Paint shade.
- Start: skip boot or close the current application.

In Sweeper, click the smiley or `Game` for a new board, or the system box to
close the window.

Solitaire (Klondike, draw one, Windows standard scoring) uses snap controls:
the D-pad jumps between piles and up/down picks how deep into a column to
grab. A picks up and drops cards (on the stock it turns a card), B sends a card
to the foundations, Select turns a card from anywhere, and moving up onto
`Game` then pressing A deals again. Once every card is face up the game
finishes itself.

The checked-in golden frames are exact 160 x 144 PyBoy output. Real-hardware
validation, minimisable windows, SRAM persistence, and optional printer support
remain release-polish milestones.
