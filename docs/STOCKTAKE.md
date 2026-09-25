# Stock-take: GB Workbench vs GBS Windows

Date: 2026-09-25

## Objective

Measure how far the clean-room rebuild (`agent/rebuild-gbc-workbench`) is from
the polished Windows 3.1 feel of RubenRetro's GBS Windows, then close the
foundation-level gaps before starting Solitaire.

## Method

- Built the rebuild reproducibly with GBDK 4.5.0; its ROM hash matched
  `docs/QA_REPORT.md` exactly and all smoke and golden-frame tests passed.
- Compared the nine native 160 x 144 frames with published photos of the GBS
  Windows cartridge (Retro Dodo review) and the CC0 Windows 3.1 Program
  Manager reference in `docs/design/`.
- GBS Windows is a closed commercial GB Studio product. It is a behavioural
  reference only; no code, art or music from it is used here.

## Reference: what GBS Windows ships

Boot beeps and a DOS driver sequence; a Program Manager with Accessories and
Games groups; Paint (tool/colour column, Game Boy Printer output); Piano;
Media Player with four tracks that keep playing in the background; Sweeper
with three board sizes; Cannon Defense. Applications open as windows over the
Program Manager. There is no Solitaire.

## Scorecard (15 visible 3.1-feel criteria)

| # | Criterion | GBS Windows | Rebuild before | This foundation PR |
|---|---|---|---|---|
| 1 | Mixed-case proportional system font | Yes | No (8x8 capitals) | Yes |
| 2 | White client areas and menu bar | Yes | No (grey) | Yes |
| 3 | Menu bar separated from the client | Yes | No | Yes |
| 4 | Centred titles with active/inactive styles | Yes | Partial | Yes |
| 5 | Win 3.1 title buttons (system box, down/up arrows) | Yes | No | Yes |
| 6 | Apps open as windows over the Program Manager | Yes | No | Yes (Sweeper, Piano, Media) |
| 7 | Minesweeper chrome: LED counters, smiley, raised cells | Yes | No | Yes |
| 8 | Piano with white and black keys | Yes | Crude grid | Yes |
| 9 | Media Player display and transport buttons | Yes | Text only | Yes |
| 10 | Paint colour/tool column | Yes | Partial | Partial (swatches) |
| 11 | Arrow pointer with tail | Yes | Triangle | Yes |
| 12 | Minimisable/movable windows | Partial | No | No |
| 13 | Sweeper board sizes | 3 | 1 | 1 |
| 14 | Game Boy Printer / saves | Printer | No | Battery SRAM available, unused |
| 15 | Cannon scenery art | Yes | Minimal | Minimal |

Distance: the rebuild met 0 of 15 criteria fully (2 partial). After this PR
it meets 10 fully and 1 partially. Functionally it already matched the GBS app
line-up; the gap was almost entirely presentation.

## Where the rebuild is already ahead

- Reproducible build with header, checksum and cartridge-type verification.
- Host unit tests, deterministic emulator smoke journeys and pixel-exact
  golden frames.
- MBC5 with automatic banking: ROM bank 0 dropped from 90% to 46% full, and
  the cartridge now has room for Solitaire, saves and more apps.

## Remaining backlog (after this PR)

1. Solitaire (Klondike) - the planned improvement over GBS Windows.
2. Sweeper Beginner/Intermediate/Expert sizes via the Game menu.
3. Paint tool column (pencil, line, fill) and SRAM-backed save.
4. Real minimise/restore for application windows.
5. Cannon scenery pass (city skyline, colour).
6. Game Boy Printer output from Paint.
7. Real-hardware and second-emulator smoke matrix.
