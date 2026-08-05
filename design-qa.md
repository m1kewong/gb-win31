# Desktop option 2 design QA

## Comparison target

- Source visual truth: `docs/design/desktop-option-2-selected-160x144.png`
- Rendered implementation: `docs/design/desktop-option-2-final.png`
- Full-view evidence: `docs/design/desktop-option-2-comparison-4x.png`
  (source left, implementation right)
- Focused evidence: `docs/design/desktop-option-2-detail-comparison-4x.png`
  (group chrome, icons, captions, and selection at 4x nearest-neighbor zoom)
- Viewport and state: native Game Boy Color `160 x 144`, Program Manager desktop,
  Accessories active, Paint selected
- Density normalization: both source and implementation are `160 x 144` at
  1 hardware pixel per pixel. Comparison evidence is enlarged 4x with nearest
  neighbor only; no resampling is used for the judgment.

## Findings

No actionable P0, P1, or P2 differences remain.

- Fonts and typography: the desktop-only 3 x 5 uppercase face uses a 4-pixel
  advance. Every caption owns a separate 32-pixel-wide four-tile cell, so
  `PAINT`, `PIANO`, `MEDIA`, `SWEEPER`, and `CANNON` remain distinct. Title and
  menu hierarchy are legible at native resolution.
- Spacing and layout rhythm: the implementation preserves the selected
  full-screen Program Manager, menu row, three-item Accessories group, and
  two-item Games group. Icon centers are evenly distributed and neither text
  nor art overwrites a frame tile.
- Colors and tokens: cyan desktop, gray window surfaces, royal-blue active
  titles/selection, inactive gray title, black text, and white selected text
  preserve the selected direction and Windows 3.1 hierarchy.
- Image and icon fidelity: all five 16 x 16 icons are sharp at 1x, use the same
  four-color VGA-like family, and express the same app subjects. Their exact
  pixels intentionally differ from the concept because the ROM uses original
  clean-room GBC tile art.
- Copy and content: app captions match their launch targets. `FILE OPTIONS
  WINDOW HELP` intentionally adds the authentic Program Manager Window menu
  while fitting on one native row.
- States and affordances: the pointer plus reversed caption communicates
  selection without the former extra marker. Each icon/caption cell has a
  32 x 32 hit target; Select cycles all five items, crossing groups updates the
  active title, and A launches the hovered app.
- Accessibility and resilience: selection has both pointer and contrast cues,
  active text contrast is high, and all content remains inside the fixed
  20 x 18 hardware tile viewport. Responsive behavior is not applicable to a
  fixed-resolution GBC screen.

## Comparison history

1. Initial integration exposed a P2 pointer-placement mismatch: the pointer
   covered the Paint icon rather than sitting beside the selected caption.
   It was moved to the lower-right of Paint's 32-pixel cell while retaining the
   hover state.
2. The next text pass exposed a P2 optical ambiguity in the packed `W`, most
   visible in `WINDOW` and `SWEEPER`. The five-row glyph was reshaped with a
   pointed lower stroke.
3. Post-fix evidence is the full-view and focused comparison listed above.
   Native capture, 4x inspection, exact-frame regression, and interaction smoke
   now show no remaining P0/P1/P2 issue.

## Follow-up polish

- P3: the deliberately tiny 3 x 5 font is more angular than the concept's
  display lettering, but this is an accepted readability/VRAM tradeoff at
  160 x 144.
- P3: minimize/maximize buttons remain visual chrome pending the separate
  window-manager milestone.

final result: passed
