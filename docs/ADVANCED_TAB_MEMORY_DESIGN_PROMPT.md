# Design Prompt: "The Memory"-Style Advanced Tab for AVIATORKEYZ

Use this as the brief for redesigning every remaining page of the Advanced tab
(Source, Filter/Envelopes, Voice, LFOs, Mod Matrix, Phrase, FX Routing,
Performance Macros) in the same interaction language already shipped on the
**Texture Engine** page. Texture is the reference implementation — match it,
don't reinvent it.

## 1. Reference implementation (already built — copy this pattern)

- `Source/GUI/Advanced/EffectCell.h/.cpp` — a large click+drag tile bound to
  one APVTS parameter. Has: big title (top-left), lock icon (top-right,
  click to freeze), centered live-fill value meter, drag hint (bottom-left),
  always-visible value readout (bottom-right), native hover tooltip,
  double-click-to-reset-to-default. Drag anywhere on the tile (not just a
  tiny knob hitbox) to change the value — that's the single most important
  affordance fix versus the old `BracketValueBox` grid.
- `EffectCell::Format` (`percent`, `semitones`, `pan`) controls how the
  value readout is formatted. Extend this enum rather than building a
  parallel component when a new format is needed (e.g. `hz`, `ms`,
  `decibels`, `integer` — mirror `BracketValueBox::Format`'s formatting
  logic for those cases).
- `TextureSectionComponent.cpp` shows the integration pattern: a
  `makeCell(...)` local helper that builds an `EffectCell` with a real,
  specific tooltip sentence (not generic boilerplate), pushed into a
  `std::vector<std::unique_ptr<EffectCell>>`, laid out in a simple
  row/column grid with `juce::Rectangle` math (no need to reuse
  `CockpitLayout::layoutGrid`).
- Visual language: `AviatorTokens` cyan-on-navy HUD palette
  (`instrumentCyan()`, `champagneGold()`, `mfdAmber()`, `hud()`/`hudBold()`
  "Share Tech Mono" fonts), flat dark background (`0xff0a1424` /
  `0xff05080b`), thin cyan borders that brighten on hover/drag. This is
  intentionally closer to The Memory's stark dark-blue minimalism than the
  champagne/piano-black `DesignTokens` used on the Main page.
- Tooltips render because `AviatorKeyzEditor` owns a
  `juce::TooltipWindow tooltipWindow { this, 500 };` member. Don't add
  another one — every `EffectCell` just needs `setTooltip(...)`, which
  works because `EffectCell` inherits `juce::SettableTooltipClient`.

## 2. The one missing interaction: click-to-cycle for bool/choice params

Texture has no bool/choice params left to redesign (FREEZE, REVERSE, etc.
already live as toggle buttons on the Main page). Every other section is
full of `AudioParameterBool` and `AudioParameterChoice` params that need a
**second tile type** before this pattern can spread further:

Build `EffectCellChoice` (or extend `EffectCell` with a `Mode` flag) that:
- Has the same visual shell (title, lock, drag hint, tooltip, double-click
  reset) as `EffectCell`.
- Replaces the drag-to-set-continuous-value behavior with **click anywhere
  on the tile to cycle to the next option** (bool = toggle; choice = wrap
  through `AudioParameterChoice::choices` in order).
- Replaces the fill-meter with the current option's name, large and
  centered (e.g. "SAW", "LP", "LEGATO") — this is the literal behavior
  described in the user's reference screenshot's tooltip: *"Click to cycle:
  Regular / Random."*
- Still binds through APVTS the same safe way: for bool params use
  `ButtonAttachment`-equivalent logic (just flip via
  `param->setValueNotifyingHost(param->getDefaultValue() > 0.5f ? 0.f :
  1.f)` or toggle the underlying normalised value); for choice params step
  `(currentIndex + 1) % numChoices` and convert back through
  `getNormalisableRange().convertFrom0to1(...)`.

Every section below is annotated with which tile type each parameter needs:
**[DRAG]** = `EffectCell`, **[CYCLE]** = `EffectCellChoice`.

## 3. Full parameter inventory (grounded in `AdvancedParameterLayout.cpp`)

### Source (Oscillators)
Two oscillators, each needs 6 tiles + a shared blend tile = 13 tiles total.

| Param ID | Tile | Title | Range/Choices | Tooltip seed |
|---|---|---|---|---|
| `osc1_type` / `osc2_type` | CYCLE | OSC1 TYPE / OSC2 TYPE | Saw, Square, Triangle, Sine, Noise, Wavetable, FM, Chord | "Click to cycle through oscillator waveforms." |
| `osc1_tune` / `osc2_tune` | DRAG (semitones) | TUNE | -24..24 st | "Drag up/down to transpose this oscillator in semitones." |
| `osc1_fine` / `osc2_fine` | DRAG (cents — extend Format) | FINE | -100..100 cents | "Drag up/down for fine pitch tuning in cents." |
| `osc1_shape` / `osc2_shape` | DRAG (percent) | SHAPE | 0..1 | "Drag up/down to morph the oscillator's wave shape." |
| `osc1_level` / `osc2_level` | DRAG (percent) | LEVEL | 0..1 | "Drag up/down to set this oscillator's volume." |
| `osc1_pan` / `osc2_pan` | DRAG (pan) | PAN | -1..1 | "Drag up/down to set this oscillator's stereo position." |
| `source_blend` | DRAG (percent) | BLEND | 0..1 | "Drag up/down to crossfade between oscillator 1 and 2." |

### Filter
| Param ID | Tile | Title | Range | Tooltip seed |
|---|---|---|---|---|
| `filter_enabled` | CYCLE (bool) | FILTER | Off/On | "Click to turn the filter on or off." |
| `filter_cutoff` | DRAG (hz — extend Format) | CUTOFF | 20..20000 Hz | "Drag up/down to set the filter cutoff frequency." |
| `filter_resonance` | DRAG (percent) | RESO | 0..1 | "Drag up/down to set filter resonance." |
| `filter_type` | CYCLE | TYPE | LP, HP, BP, Notch | "Click to cycle filter type." |
| `filter_drive` | DRAG (percent) | DRIVE | 0..1 | "Drag up/down to drive the filter into saturation." |

### Envelopes
| Param ID | Tile | Title | Range | Tooltip seed |
|---|---|---|---|---|
| `env_amp_decay` | DRAG (ms/s — extend Format) | AMP DECAY | 0..10s | "Drag up/down to set the amp envelope decay time." |
| `env_amp_sustain` | DRAG (percent) | AMP SUSTAIN | 0..1 | "Drag up/down to set sustain level." |
| `env_flt_attack` | DRAG (ms/s) | FLT ATTACK | 0.001..10s | "Drag up/down to set how fast the filter envelope opens." |
| `env_flt_decay` | DRAG (ms/s) | FLT DECAY | 0.001..10s | "Drag up/down to set filter envelope decay time." |
| `env_flt_sustain` | DRAG (percent) | FLT SUSTAIN | 0..1 | "Drag up/down to set filter envelope sustain level." |
| `env_flt_release` | DRAG (ms/s) | FLT RELEASE | 0.001..30s | "Drag up/down to set filter envelope release time." |
| `env_flt_amount` | DRAG (percent, bipolar) | FLT ENV AMT | -1..1 | "Drag up/down to set how much the envelope modulates the filter." |

### Voice / Global
| Param ID | Tile | Title | Range | Tooltip seed |
|---|---|---|---|---|
| `velocity_sensitivity` | DRAG (percent) | VEL SENS | 0..1 | "Drag up/down to set how much velocity affects volume." |
| `voice_polyphony` | DRAG (integer — extend Format) | POLY | 1..16 voices | "Drag up/down to set the maximum number of voices." |
| `voice_glide_mode` | CYCLE | GLIDE | Off, Legato, Always | "Click to cycle glide mode." |
| `voice_play_mode` | CYCLE | PLAY MODE | Poly, Mono, Legato | "Click to cycle polyphony mode." |
| `output_limiter` | CYCLE (bool) | LIMITER | Off/On | "Click to turn the output limiter on or off." |

### Texture Engine — **already done**, no further work needed.

### LFOs (×3, each identical 5-tile set = 15 tiles)
| Param ID pattern | Tile | Title | Range | Tooltip seed |
|---|---|---|---|---|
| `lfoN_rate` | DRAG (hz) | RATE | 0.01..20 Hz | "Drag up/down to set LFO speed." |
| `lfoN_depth` | DRAG (percent) | DEPTH | 0..1 | "Drag up/down to set how strongly this LFO modulates its target." |
| `lfoN_shape` | CYCLE | SHAPE | Sine, Square, Triangle, Ramp Up, Ramp Down, Random | "Click to cycle LFO waveform." |
| `lfoN_sync` | CYCLE (bool) | SYNC | Free/Synced | "Click to sync this LFO to the DAW tempo." |
| `lfoN_phase` | DRAG (percent) | PHASE | 0..1 | "Drag up/down to offset the LFO's starting phase." |

### Mod Matrix (8 identical rows = 32 tiles, or keep as a routing-table UI)
Each row: `modN_on` (CYCLE bool, ON/OFF), `modN_source` (CYCLE, source
list from `ModMatrix::sourceNames()`), `modN_dest` (CYCLE, dest list from
`ModMatrix::destNames()`), `modN_amount` (DRAG percent, bipolar-aware).
This section is structurally a table, not a tile grid — consider keeping
`ModRoutingHub`'s row layout but swapping its `ComboBox`/`TextButton`
controls for `EffectCellChoice` tiles sized to fit a row, so the same
click-to-cycle language is consistent without forcing an awkward 8-row ×
4-tile grid.

### Phrase
| Param ID | Tile | Title | Range | Tooltip seed |
|---|---|---|---|---|
| `phrase_enabled` | CYCLE (bool) | PHRASE | Off/On | "Click to turn phrase mode on or off." |
| `phrase_tempo_sync` | CYCLE (bool) | TEMPO SYNC | Off/On | "Click to sync phrase playback to the DAW tempo." |
| `phrase_key_sync` | CYCLE (bool) | KEY SYNC | Off/On | "Click to retrigger the phrase from the start on each new key." |
| `phrase_trigger_mode` | CYCLE | TRIGGER | Hold, Trigger, Gate | "Click to cycle how the phrase is triggered." |
| `phrase_loop` | CYCLE (bool) | LOOP | Off/On | "Click to loop the phrase continuously." |
| `phrase_start` | DRAG (percent) | START | 0..1 | "Drag up/down to set where playback starts in the phrase." |
| `phrase_length` | DRAG (percent) | LENGTH | 0..1 | "Drag up/down to set how much of the phrase plays." |
| `phrase_pitch` | DRAG (semitones) | PITCH | -24..24 st | "Drag up/down to transpose the phrase." |

### FX Routing
| Param ID | Tile | Title | Range | Tooltip seed |
|---|---|---|---|---|
| `fx_reverb_on` | CYCLE (bool) | REVERB | Off/On | "Click to turn reverb on or off." |
| `fx_reverb_damp` | DRAG (percent) | DAMP | 0..1 | "Drag up/down to set reverb high-frequency damping." |
| `fx_edits_on` | CYCLE (bool) | PRESET FX | Off/On | "Click to apply or bypass this preset's saved FX edits." |
| `fx_delay_on` | CYCLE (bool) | DELAY | Off/On | "Click to turn delay on or off." |
| `fx_delay_time` | DRAG (ms/s) | TIME | 0.01..2s | "Drag up/down to set delay time." |
| `fx_delay_feedback` | DRAG (percent) | FEEDBACK | 0..1 | "Drag up/down to set delay feedback amount." |
| `fx_delay_mix` | DRAG (percent) | MIX | 0..1 | "Drag up/down to set delay wet/dry mix." |
| `fx_delay_sync` | CYCLE (bool) | SYNC | Off/On | "Click to sync delay time to the DAW tempo." |
| `fx_chorus_on` | CYCLE (bool) | CHORUS | Off/On | "Click to turn chorus on or off." |
| `fx_chorus_rate` | DRAG (percent) | RATE | 0..1 | "Drag up/down to set chorus modulation rate." |
| `fx_chorus_depth` | DRAG (percent) | DEPTH | 0..1 | "Drag up/down to set chorus modulation depth." |
| `fx_chorus_mix` | DRAG (percent) | MIX | 0..1 | "Drag up/down to set chorus wet/dry mix." |
| `fx_lofi_on` | CYCLE (bool) | LO-FI | Off/On | "Click to turn lo-fi degradation on or off." |
| `fx_lofi_amount` | DRAG (percent) | AMOUNT | 0..1 | "Drag up/down to set lo-fi bit/sample-rate reduction amount." |
| `fx_dist_on` | CYCLE (bool) | DIST | Off/On | "Click to turn distortion on or off." |
| `fx_dist_drive` | DRAG (percent) | DRIVE | 0..1 | "Drag up/down to set distortion drive amount." |

### Performance Macros
| Param ID | Tile | Title | Range | Tooltip seed |
|---|---|---|---|---|
| `perf_macro_1..4` | DRAG (percent) | MACRO 1..4 | 0..1 | "Drag up/down — this macro can be assigned to modulate multiple parameters at once." |

## 4. Build order recommendation

1. Add `EffectCellChoice` (or the `Mode` extension) — every other section
   depends on it. Reuse `EffectCell`'s shell/paint code where possible to
   avoid duplicating the lock/tooltip/border logic.
2. Extend `EffectCell::Format` with `hz`, `ms`/seconds, `cents`,
   `decibels`, `integer` (mirror `BracketValueBox::formatValue()` — don't
   re-derive the formatting rules from scratch).
3. Redesign Source + Filter + Envelopes together (they live on one
   conceptual "synth" page) — 25 tiles.
4. Redesign Voice/Global (5 tiles) — small, quick.
5. Redesign LFOs (15 tiles, identical ×3 — write one `addLfoRow` helper).
6. Redesign Phrase (8 tiles).
7. Redesign FX Routing (16 tiles, group visually by effect: Reverb / Delay
   / Chorus / Lo-Fi / Dist).
8. Redesign Mod Matrix last — it's structurally different (a routing
   table, not a uniform grid) and benefits most from having the other
   tile types already proven out first.
9. Performance Macros (4 tiles) can piggyback onto whichever page makes
   layout sense, or stay as its own strip — `PerformanceMacroStrip` is
   already separate from the tab grid.

## 5. Constraints to keep in mind

- Every tile must stay bound through `AudioProcessorValueTreeState`
  attachments (`SliderAttachment` for drag tiles, a manual
  `setValueNotifyingHost` step-cycle for choice tiles) — never bypass
  APVTS, or host automation/undo breaks.
- Don't duplicate `TEX_FREEZE` / `TEX_REVERSE` / `TEX_GRAIN_PITCH` /
  `TEX_GRAIN_PAN` here — confirmed those already live on the Main page's
  `TextureSectionComponent` toggles and should stay there.
- Keep `EffectCell`/`EffectCellChoice` files in `Source/GUI/Advanced/` and
  remember to add any new `.cpp` file to `CMakeLists.txt`'s
  `target_sources` list — it is an explicit file list, not a glob, and a
  missing entry silently fails to link rather than erroring at configure
  time.
