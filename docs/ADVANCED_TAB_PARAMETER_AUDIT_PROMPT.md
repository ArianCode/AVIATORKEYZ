# Audit Prompt: Verify Every Knob Is Bound and Formatted Correctly (Pre-Audio-Test Pass)

Goal: catch every parameter-binding, range, and display bug in the Advanced/
Performance tab **in code**, before doing any audio-domain testing in a host.
Three real bugs were already found this way — fix those first, then run the
systematic pass below on everything else.

## 0. Bugs already found and confirmed (fix these now)

### Bug 1 — `SRC_ORIGINAL_BPM` displays "12000%" instead of "120"
- **Where:** `Source/GUI/Advanced/AdvancedPageContent.cpp:29`
  ```cpp
  addCell (P::SRC_ORIGINAL_BPM, "ORIG BPM", "Original sample BPM.", Fmt::percent);
  ```
- **Why it's wrong:** `SRC_ORIGINAL_BPM`'s real range is `NR (40.f, 240.f, 0.1f)`
  with default `120.f` (`PerformanceParameterLayout.cpp:30-31`) — this is a raw
  BPM value, not a 0..1 normalized one. `EffectCellFormat::formatValue`'s
  `Format::percent` case has no keyword match for "bpm" in its whitelist, so it
  falls through to the generic `roundToInt (v * 100.f) + "%"` path — i.e. it
  multiplies the raw BPM by 100. At the default value of 120 that's exactly
  `12000%`, which matches the screenshot precisely.
- **Fix:** change the format argument to `Fmt::integer` (already exists in
  `EffectCellFormat::Format`):
  ```cpp
  addCell (P::SRC_ORIGINAL_BPM, "ORIG BPM", "Original sample BPM.", Fmt::integer);
  ```

### Bug 2 — `CHOP_SMOOTH` value readout is disconnected from its real range
- **Where:** `AdvancedPageContent.cpp:38`
  ```cpp
  addCell (P::CHOP_SMOOTH, "SMOOTH", "Crossfade smoothing.", Fmt::percent);
  ```
- **Why it's wrong:** `CHOP_SMOOTH`'s range is `NR (0.001f, 0.1f, 0.001f)`,
  default `0.01f` — a time value in seconds (1ms–100ms), not 0..1. "smooth"
  isn't in `formatValue`'s percent whitelist, so it falls to the same
  `v * 100` fallback. At the default this happens to print "1%", which looks
  plausible, but it's not actually showing position-in-range: at the param's
  true maximum (0.1) it would still only print "10%" instead of "100%", so
  the readout and the tile's fill meter will visibly disagree once someone
  drags this near its top.
- **Fix:** use a time format instead of percent:
  ```cpp
  addCell (P::CHOP_SMOOTH, "SMOOTH", "Crossfade smoothing.", Fmt::ms);
  ```
  (`Format::ms` already formats <1000 as "N ms", so 0.01s → "10 ms", correct.)

### Bug 3 — MACROS+SPACE tiles overlap the macro strip
- **Where:** `AdvancedPageContent.cpp`, `layoutSection()`, lines ~127-190.
- **Why it's wrong:**
  ```cpp
  int x = area.getX();
  int y = area.getY();          // <-- captured ONCE, before the switch
  ...
  case Section::macrosSpace:
      macroStrip->setVisible (true);
      macroStrip->setBounds (area.removeFromTop (120));   // mutates `area`, not x/y
      for (const char* id : { P::FX_REVERB_ON, P::REVERB_AMOUNT, P::FX_DELAY_ON, P::FX_DELAY_MIX })
          show (id);   // show() -> place() uses the STALE x/y from before removeFromTop
      break;
  ```
  `place()` closes over `x`/`y` by reference, but those were snapshotted from
  `area.getX()/getY()` *before* `area.removeFromTop(120)` ran. So the four FX
  cells (REVERB, REV MIX, DELAY, DLY MIX) get placed starting at the same `y`
  as the macro strip's top — exactly the overlap visible in the third
  screenshot (DLY MIX's tile sitting on top of/behind the macro tiles, and
  the unstyled-looking "Space"/"Tone" macro tiles bleeding into the same row
  as REVERB/DELAY).
- **Fix:** advance `y` (and reset `x`/`col`) to the bottom of the macro strip
  before placing the FX cells, e.g.:
  ```cpp
  case Section::macrosSpace:
  {
      macroStrip->setVisible (true);
      auto macroArea = area.removeFromTop (120);
      macroStrip->setBounds (macroArea);
      y = area.getY();   // <-- advance past the macro strip
      x = area.getX();
      col = 0;
      for (const char* id : { P::FX_REVERB_ON, P::REVERB_AMOUNT, P::FX_DELAY_ON, P::FX_DELAY_MIX })
          show (id);
      break;
  }
  ```
  Note: the "Space" / "Tone" titles themselves are *not* a bug — those are
  dynamic macro names assigned by `MacroMapper.cpp` (macro 3/4 get retitled
  per the active performance mode via `setMacroLabels`). Don't rename them
  back to "MACRO 3"/"MACRO 4"; just fix the overlap.

## 1. Systematic check to run on every remaining `addCell`/`addChoice` call

`AdvancedPageContent.cpp` wires ~30 parameters through `addCell`/`addChoice`.
The three bugs above were all found the same way — by reading the `Fmt::`
argument next to each call and cross-checking it against the parameter's
*actual* `NormalisableRange` in `PerformanceParameterLayout.cpp` /
`AdvancedParameterLayout.cpp`. Repeat this for every remaining cell:

1. For each `addCell (id, title, tooltip, Fmt::X)` line, grep the same `id`
   in `PerformanceParameterLayout.cpp` and `AdvancedParameterLayout.cpp` to
   find its real `NormalisableRange (min, max, ...)` and default value.
2. If `min == 0.f && max == 1.f`: `Fmt::percent` is safe (normalized ==
   percent either way).
3. If the range is **not** 0..1: check whether the param ID contains one of
   the whitelist substrings in `EffectCellFormat::formatValue`'s
   `Format::percent` case (`level`, `amount`, `mix`, `depth`, `sustain`,
   `shape`, `blend`, `spread`, `width`, `density`, `motion`, `drift`, `air`,
   `scan`, `size`, `rate` (non-lfo/chorus), `smear`, `reverb_amount`,
   `lofi`, `dist`, `damp`, `feedback`, `resonance`, `drive` (non-filter),
   `mod_...amount`). If it's on the list, `formatValue` will correctly
   normalize via `convertTo0to1` before multiplying by 100 — safe. **If it's
   not on the list and the range isn't 0..1, it's the same bug class as
   `SRC_ORIGINAL_BPM`/`CHOP_SMOOTH`** — pick the right format instead
   (`Fmt::integer`, `Fmt::hz`, `Fmt::ms`/`Fmt::seconds`, `Fmt::semitones`,
   `Fmt::cents`, `Fmt::decibels`, `Fmt::cutoff`).
4. For every `addChoice (id, title, tooltip, choiceList)` call: confirm
   `choiceList` is in **exactly** the same order as the `StringArray` passed
   to that parameter's `AudioParameterChoice` constructor in the layout
   file. A reordered local list (e.g. `kChopRates` vs. the param's real
   `StringArray`) will make the tile cycle through the right number of
   options but display/select the wrong label for the underlying index.
5. For every `EffectCell`/`EffectCellChoice`, confirm
   `mouseDoubleClick`'s reset-to-default (`param->getDefaultValue()`) lands
   on a value that actually makes sense for that control (e.g. don't let a
   "smoothing time" reset to literal `0.0` if that's not what the
   parameter's registered default is — cross-check against the `NR(...)`
   default argument, not an assumption).

## 2. Layout-overlap check (same bug class as Bug 3)

Bug 3 happened because a layout function captured `x`/`y` before mutating
the `Rectangle` they were derived from. Grep `AdvancedPageContent.cpp`,
`AdvancedPanel.cpp`, and any other `resized()`/`layoutSection()`-style method
for the same anti-pattern: a `Rectangle` (or `x`/`y` derived from one) read
*before* a `removeFromTop`/`removeFromLeft`/etc. call that's meant to make
room for something, then reused afterward as if it had been updated. Any
hit there is a near-guaranteed visual overlap bug, exactly like Bug 3.

## 3. Binding correctness (do this before any audio testing)

For each `EffectCell`/`EffectCellChoice`, confirm in code (not by ear) that:
- The `SliderAttachment`/manual-write path actually targets the same param
  ID used by the DSP engine reading it (grep the ID in
  `PluginProcessor.cpp`/the relevant `DSP/` file and confirm it's the same
  string constant from `StateSchema.h`, not a typo'd literal).
- No two parameter layout files (`AdvancedParameterLayout.cpp`,
  `PerformanceParameterLayout.cpp`) register the same `ParamID` twice —
  duplicate `RangedAudioParameter` IDs in one APVTS will assert/crash at
  construction. Quick check: `grep -c "ParamID::<id>"` across both layout
  files should return exactly 1 for any given ID.
- `getNormalisableRange().convertTo0to1(...)` based fill-meter math
  (`EffectCell::getNormalisedValue()`) agrees visually with the value
  readout text for a few manual drags across each tile's full range —
  this is the cheapest way to catch the Bug-1/Bug-2 class without an audio
  signal at all, just by reading the numbers on screen against known knob
  positions (min, default, max).

## 4. Sign-off checklist

- [ ] Fix Bug 1 (`SRC_ORIGINAL_BPM` → `Fmt::integer`)
- [ ] Fix Bug 2 (`CHOP_SMOOTH` → `Fmt::ms`)
- [ ] Fix Bug 3 (advance `x`/`y` past the macro strip in `macrosSpace`)
- [ ] Re-grep every `addCell`/`addChoice` call against Section 1's checklist
- [ ] Re-grep every `resized()`/`layoutSection()` for Section 2's
      capture-before-mutate pattern
- [ ] Confirm no duplicate `ParamID` registrations across the two layout
      files (Section 3)
- [ ] Only after all of the above: move to actual audio-domain testing
      (load in a host, automate each parameter, confirm DSP responds)
