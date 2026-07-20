# Design Prompt: Fix the Advanced "Synth" Page Layout (3×3 Grid + Bottom-Third Visualizer)

## 0. What's actually broken right now (read this first)

The screenshot shows real bugs, not just a busy layout:

1. **Every tile has a phantom rotary dial bleeding through it.** Look at
   TUNE, FINE, SHAPE, CUTOFF, RESO, etc. — each shows a horizontal bar
   *and* a circular arc/dot rendered on top of it. That circular artifact
   is JUCE's **default `Slider` look-and-feel painting itself**, because
   `EffectCell`'s internal hidden slider is added with
   `addAndMakeVisible (slider)` instead of being hidden. Compare to
   `BracketValueBox`, which does this correctly:
   `hiddenSlider.setVisible (false); addChildComponent (hiddenSlider);`
   **Fix:** in `EffectCell`'s constructor, change
   `addAndMakeVisible (slider);` → `slider.setVisible (false);
   addChildComponent (slider);`. The slider still receives forwarded mouse
   events via `slider.mouseDown(e.getEventRelativeTo(&slider))` etc. (that
   code path doesn't depend on the slider being visible), so dragging
   keeps working — you just stop seeing its default rotary knob. This one
   fix removes the visual clutter from every single tile at once.

2. **Sections overlap each other with no shared layout container.** FLT
   ATK / FLT DEC / FLT SUS sit on top of the LFO1 sine-wave graphic and
   LFO1's RATE/DEPTH/PHASE row. OSC1's BLEND/GAIN/VEL SENS row sits on top
   of OSC2's TUNE/FINE row. This means `AdvancedPageContent`'s `resized()`
   is placing these groups with overlapping or unclamped `Rectangle`
   math — there's no single grid container owning fixed, non-overlapping
   cells for each function group. That's the real fix needed here: stop
   positioning each group independently and give the whole page one grid.

## 1. Target layout

Split the Advanced "Synth" page (everything except the page-tab bar and
the page-select footer — TEXTURE ENGINE / MATRIX / FX ROUTING / UTILITY)
into two stacked regions:

- **Upper 2/3 of the available height:** a strict **3×3 grid, 9 equal
  cells**, no overlap, no bleed between cells. Each cell is one
  self-contained function box with its own border, header label, and
  parameter rows. Nothing from one cell may render outside its own
  bounds.
- **Bottom 1/3:** the Texture visualizer, full width, exactly like it
  already looks in the Texture Engine page — this becomes the page's
  hero element instead of being squeezed into a corner.

### The 9 boxes (left → right, top → bottom)

| Cell | Box | Parameters inside |
|---|---|---|
| 1,1 | **OSC 1** | osc1_type (cycle), osc1_tune, osc1_fine, osc1_shape, osc1_level, osc1_pan |
| 1,2 | **FILTER** | filter_enabled (cycle), filter_cutoff, filter_resonance, filter_type (cycle), filter_drive |
| 1,3 | **LFO 1** | lfo1_rate, lfo1_depth, lfo1_shape (cycle), lfo1_sync (cycle), lfo1_phase |
| 2,1 | **OSC 2** | osc2_type (cycle), osc2_tune, osc2_fine, osc2_shape, osc2_level, osc2_pan |
| 2,2 | **AMP / FILTER ENV** | env_amp_decay, env_amp_sustain, env_flt_attack, env_flt_decay, env_flt_sustain, env_flt_release, env_flt_amount |
| 2,3 | **LFO 2** | lfo2_rate, lfo2_depth, lfo2_shape (cycle), lfo2_sync (cycle), lfo2_phase |
| 3,1 | **VOICE / GLOBAL** | source_blend, velocity_sensitivity, voice_polyphony, voice_glide_mode (cycle), voice_play_mode (cycle), output_limiter (cycle) |
| 3,2 | *(reserved / balance cell — see note below)* | — |
| 3,3 | **LFO 3** | lfo3_rate, lfo3_depth, lfo3_shape (cycle), lfo3_sync (cycle), lfo3_phase |

**Note on cell 3,2:** the current page has 8 natural function groups
(OSC1, OSC2, FILTER, AMP+FLT ENV combined, LFO1, LFO2, LFO3, VOICE), which
only fills 8 of 9 cells. Either (a) split AMP ENV and FLT ENV into two
separate boxes — giving 9 *clean* groups instead of forcing an empty
cell — or (b) leave 3,2 as a dedicated "FILTER CURVE" visual (the curve
graph currently crammed above the FILTER box) so it gets real room
instead of being squeezed into a thin strip at the top of the page.
Recommend **(a)**: split env into "AMP ENV" (cell 2,2: amp_decay,
amp_sustain) and "FLT ENV" (cell 3,2: flt_attack, flt_decay, flt_sustain,
flt_release, flt_amount) — that's a true 9-for-9 match and reads cleaner
than a combined 7-parameter box anyway.

## 2. Inside each box: clean right-border sliders

Replace the current per-parameter tile style (rotary dial + horizontal
bar drawn together, which is what causes the clutter once bug #1 above is
fixed and isolated) with **one consolidated row-list layout per box**:

- Each parameter is a single horizontal row: label on the left, current
  value readout in the middle, and a **slim vertical slider track running
  along the box's right inner edge** — one shared rail per box, with each
  row's slider thumb riding on that rail at its row's height. Think of it
  like a small mixer-strip: a column of thumbs on one continuous vertical
  track, each thumb independently draggable up/down for its own
  parameter.
- For cycle/choice parameters (osc type, filter type, LFO shape, sync,
  etc.) skip the slider rail for that row — just show the current choice
  text and a "CLICK = MODE" hint, consistent with how OSC1 TYPE / FILTER
  TYPE already render in the screenshot.
- Keep using `EffectCell`'s drag/lock/tooltip/double-click-reset
  interaction model underneath — this is a visual-density change (compact
  row + rail instead of one big tile per parameter), not a new
  interaction model. A box with 5–7 parameters should fit comfortably in
  a 3×3 grid cell at typical plugin window sizes once it's this compact.

## 3. Cleanup checklist

- [ ] Fix `EffectCell`'s slider visibility bug (Section 0.1) — do this
      first, it's one line and instantly de-clutters every existing tile.
- [ ] Replace whatever ad-hoc per-group positioning exists in
      `AdvancedPageContent::resized()` with one explicit 3×3 grid: compute
      9 equal `Rectangle<int>` cells from `getLocalBounds()` scaled to the
      upper 2/3 height, hand each cell's bounds to exactly one box
      component, and never let two boxes share or exceed their cell.
- [ ] Move/resize the Texture visualizer to occupy the full-width bottom
      1/3 strip, replacing its current cramped placement.
- [ ] Delete or fully replace any leftover absolute-positioned remnants
      from the old per-group layout (the FLT ATK/DEC/SUS cluster and LFO
      rows currently overlapping each other) — don't layer the new grid
      on top of old positioning code, remove the old code paths so there
      is only one source of truth for where each box lives.
- [ ] Confirm no box's children ever paint outside that box's own
      `Rectangle` (clip or simply trust correct non-overlapping bounds —
      but bounds must actually be correct, not just visually close).
