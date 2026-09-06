# AviatorKeyz — Changelog

All notable changes to this project will be documented here.
Format: [Version] — Date — Summary

---

## [Unreleased] — 2026-09-05 (later) — MFX rack replaces Arpeggiator + Atmosphere

### Added
- **MFX rack** (`Source/DSP/Mfx/`, `Source/GUI/FlightDeck/MfxSlotPanel.*`): two
  generic effect slots in series with a shared reverb send. Each slot: power,
  prev/next, categorised effect menu, PRESET menu, REV SEND, LEVEL, REROLL /
  SURPRISE ME / AMOUNT with per-slot locks and a 32-step undo, a 16-slot
  parameter bank relabelled per effect, and four ASSIGN rows (mod wheel,
  velocity, aftertouch, LFO 1, envelope, macro 1, note pitch).
- Effects with real DSP: Grain Cloud (the former Atmosphere engine), Sweep
  Filter, Tape Echo, Saturator, Stutter, Freeze, Bitcrusher, Compressor, Chorus,
  Space — each with four factory presets and musical randomise ranges.
- 56 new parameter IDs (`mfx1_*`, `mfx2_*`), total 318. New effects cost zero IDs.
- Mod wheel / aftertouch / last velocity tracked in `MidiHandler` for the assigns.

### Changed
- Flight Deck layout: MFX slot A where the ARPEGGIATOR was, slot B where
  ATMOSPHERE was. The arpeggiator DSP, parameters and zone UI remain in the
  tree (not shown) so they can return later.
- MAIN page: LAYER MIX "GRN" → "FX B" (slot B level + power); centre-strip
  WIDTH → REV SEND (slot A).
- Flip lever: the direction change now ramps in over 3 ms (no click).
- `ptex_*` (Atmosphere) parameters are no longer processed; states that used
  them migrate to slot B = Grain Cloud automatically.

## [Unreleased] — 2026-09-05 — Flight Deck performance view, main-page wiring, sample spec

### Added
- **FLIGHT DECK** replaces the old PERFORMANCE tab (`Source/GUI/FlightDeck/`,
  fixed 1366x860 canvas scaled below the header): ARPEGGIATOR, MANEUVER · SAMPLE
  FLIP, ATMOSPHERE and CARGO HOLD zones per the approved v3 prototype.
- `Arpeggiator` (`Source/DSP/Arpeggiator.*`, pure sequencing in `ArpPattern.h`):
  host-PPQ-synced MIDI note scheduler with up/down/up-down/random/as-played,
  1–4 octaves, rate + triplet/dotted feel, gate, swing, humanize, octave spread,
  HOLD latch (sustain pedal latches too) and an ARP TARGET switch — NOTES plays
  chromatic pitches, SLICES steps through 16 slices of the source window at the
  sample's own pitch.
- Sample flip lever: `reverse` now flips *sounding* voices in place, snapped to
  the beat grid (`flip_snap`), confined to a phrase / slice / beat window
  (`flip_window`). New notes started while reversed honour the same window.
- CARGO HOLD user sample import: drop or browse a WAV/AIFF (≤ 60 s). Tempo and
  key are estimated (`Source/State/SampleAnalysis.*`), playback policy, ORIG BPM,
  ROOT, START/END are set, and the file path round-trips through host state
  (`sampleId = user:<path>`).
- Sample-accurate MIDI: the processor now renders voices in segments between
  events instead of block-quantising every note-on.
- New parameters (13): `arp_on, arp_mode, arp_rate, arp_feel, arp_octaves,
  arp_gate, arp_swing, arp_humanize, arp_oct_spread, arp_hold, arp_target,
  flip_window, flip_snap` (total 262).
- `scripts/check_sample_spec.py` — validates / conforms source samples to the
  mix spec (WAV PCM · 48 kHz · 24-bit · stereo · peak −6…−3 dBFS · no
  normalization, no dither, never MP3/AAC). `import_factory_bank.py` now gates
  on it (`--skip-spec-check` to bypass). `tests/test_sample_spec.py`.

### Fixed
- **Envelope DECAY was inaudible**: `env_amp_decay` is stored in seconds but was
  handed to the sampler as milliseconds (max 10 ms). SUSTAIN was also ignored
  whenever DECAY was 0. Both now behave; the MAIN envelope knobs show values.
- **LAYER MIX faders did nothing**: `SynthEngine` (OSC1/OSC2) and the legacy
  `TextureEngine` (TEX) were never prepared or processed. They are now additive
  layers over the sampler. Osc level defaults dropped 0.7 → 0 so presets stay
  sampler-only; projects saved before this build are migrated to 0 as well.
- **FILTER panel did nothing**: `FilterProcessor` was never in the signal path.
  It now runs after the voice sum when `filter_enabled`. The MAIN panel gained
  HP / LP chips, an on LED, a cutoff readout, and dragging the curve engages the
  filter (right / alt-drag flips HP ↔ LP). `filter_type` default is now HP.
- `pitch_align.py` wrote 16-bit mono; it now writes 24-bit dual-mono per spec.

### Changed
- MAIN centre strip: LOFI cell replaced by a REVERSE (FWD ▶▶ / ◀◀ REV) cell
  mirroring the Flight Deck lever.
- Chop slices (and arp slice steps) apply to a voice from its first sample
  instead of one block late.
- Debug snapshot harness: `AVIATORKEYZ_SNAPSHOT_VIEW=performance` renders the
  Flight Deck.

### Known gaps
- The four `perf_macro_*` knobs no longer have an on-screen home (the old
  MACROS + SPACE tab went with the advanced page); they remain automatable.
- The MFX slot panel prototype is queued to replace the ARPEGGIATOR zone.

## [Unreleased] — 2026-09-02 — Host state lifecycle fix

### Fixed
- Reopening a saved project (and starting an offline render) played a sine fallback
  instead of the preset's sample. `prepareToPlay` skipped the reload because
  `setStateInformation` had already matched `loadedSampleId`, while
  `releaseResources` had nulled the sampler snapshot. See FLSI-008 in
  `known-host-issues.md`.
- `amp sustain == 0` no longer counts as invalid sampler state. It used to make a
  legitimate percussive patch report a failed load and re-decode the WAV inside
  `prepareToPlay` on every transport start.

### Changed
- `SamplerEngine::hasLoadedSample()` split out of `validateCurrentState()`, so
  load/prepare lifecycle decisions no longer depend on envelope settings
- Unit-test target links the real `Source/PluginProcessor.cpp` with
  `AVIATORKEYZ_HEADLESS_TESTS=1` (no GUI layer), so host state tests exercise the
  shipped code instead of a copy of it

### Tests
- `tests/HostStateRoundtripTests.cpp` — save/restore, FL reopen ordering, repeated
  release/prepare cycles, and the zero-sustain case. Verified to fail against the
  pre-fix processor.
- Validated in FL Studio on macOS (2026-09-02): render path confirmed fixed on a
  universal build stamped `5727cec8b4`

---

## [Unreleased] — 2026-08-19 — MIDI keytrack (working tree; not committed)

### Changed
- Runtime MIDI pitch follows `sourceSettings.keytrack` (not playback-mode-only)
- Ensembles one-shots (including Electric Guitar Fading) use chromatic resample + keytrack; phrases stay phrase
- Advanced source section exposes a KEY TRACK control
- Category policy (C++ and Python) sets `SRC_KEYTRACK` from chromatic mode

### Tests
- `tests/PitchTrackingTests.cpp` (untracked until the keytrack change is committed)

---

## [Unreleased] — 2026-07-20 — Aviation cockpit MAIN interface

### Added
- New MAIN interface (`Source/GUI/Aviation/`): full-width cockpit view on a
  fixed 1647x955 design canvas, scaled proportionally by the editor
  (default window 1100x638, fixed aspect ratio, resizable 824–1976 wide)
- Top-right preset dropdown as the preset selector (category presets,
  favorites starred, current preset marked with a gold suitcase) with
  overhead-luggage-compartment menu styling
- Center dashboard: RPM (host BPM) / KEY / TUNE readouts, radar instruments,
  aircraft blueprint, GLOBALS output gain, LIMITER toggle, and
  LOFI / STEREO / DYNAMICS / WIDTH / HUMANIZE cells
- Cockpit glass side panels: velocity curve, layer mix, filter curve,
  amplitude envelope (all bound to existing parameters)
- Macro deck: eight LED-ring macro knobs mirrored around the Aviation brand
  block (Glide, Gain, Brightness, Reverb, Tone, Filter, Attack, Release)
- Live status bar (sample rate, host BPM, A/B state compare) and persisted
  preset favorites
- Debug-only screenshot harness (`AVIATORKEYZ_SNAPSHOT_PATH`) rendering the
  canonical 1647x955 UI for visual regression checks

### Changed
- Loaded sample region is the authoritative pitch/tempo source; root note and
  inferred original BPM sync back into APVTS on preset load
- Legacy MainPanel/ViewModeTabBar removed from the visible hierarchy
  (still compiled); PERFORMANCE tab shows the Advanced panel

---

## [0.1.0] — 2026-04-15 — M0: Initial Scaffold

### Added
- CMakeLists.txt with JUCE 7.0.12 via FetchContent
- VST3 plugin target configured as synth instrument (IS_SYNTH=TRUE)
- Plugin manufacturer code: `Avkz`, plugin code: `Avk1`
- `Source/State/StateSchema.h` — all parameter IDs (v1), schema version constant, category names
- `PluginProcessor` — APVTS with 9 parameters, state save/load with schema versioning
- `PluginEditor` — resizable editor (700×404 min, 1800×1040 max), dark gold placeholder UI
- `DSP/SamplerEngine` — stub (M1)
- `DSP/ToneShaper` — stub (M2)
- `DSP/SmearProcessor` — stub (M2)
- `DSP/GlideEngine` — portamento engine with linear ramp, per-note pitch tracking
- `DSP/ReversePlayer` — read-increment sign utility for reversed sample playback
- `State/PresetManager` — stub with category list (M3)
- `GUI/LuxuryLookAndFeel` — gold/dark palette, base colour scheme set
- `GUI/KnobComponent` — reusable APVTS-attached rotary knob
- `GUI/WaveformDisplay`, `MainPanel`, `HeaderBar`, `PresetBrowser` — stubs (M3/M4)
- `MIDI/MidiHandler` — stub with note/CC routing structure (M1)
- `scripts/build_windows.bat` — Release + Debug builds, install instructions
- `.gitignore`
- `README.md`, `ARCHITECTURE.md`, `TESTPLAN.md`, `CHANGELOG.md`, `TODO.md`, `known-host-issues.md`

### Parameters introduced (v1 — IDs are now fixed)
- `input_gain`, `output_gain`, `reverse`, `glide_time`, `smear`, `tone`
- `reverb_amount`, `reverb_size`, `stereo_width`

---

## [Unreleased] — 2026-06-05 — Factory bank + certification prep

### Added
- Per-preset factory bank (~110 licensed WAVs + XML presets via `import_factory_bank.py`)
- `PitchProbe` utility for pitch alignment tests
- Photo-anchored cockpit controls wired from `CockpitZones`
- Host project state: preset category/name, sampleId, rootNote persisted
- `scripts/run_m5_smoke.sh`, `scripts/notarize_macos.sh`, `docs/M5_CERTIFICATION.md`

### Changed
- Python/C++ tests updated for per-preset factory model
- `PresetManagerTests` and `PitchAlignmentTests` wired in CMake
- JUCE 8.0.9 (was 7.0.12 in v0.1.0 scaffold)

---

## [Unreleased] — M1: Core Sampler Engine
- Polyphonic sample playback
- MIDI note/CC routing
- Velocity scaling
- Sample map loading

## [Unreleased] — M2: Creative Engine
- Reverse mode (buffer read direction flip per voice)
- Glide (portamento pitch ramp)
- Smear (transient blur/diffusion)
- Tone (tilt EQ)

## [Unreleased] — M3: Preset System
- Factory preset library (10 categories, 5+ presets each)
- User preset save/load
- Preset browser UI

## [Unreleased] — M4: Premium UI
- LuxuryLookAndFeel full implementation
- Custom rotary knob rendering
- Waveform display
- HeaderBar with logo
- Full MainPanel layout

## [Unreleased] — Platform priority

- **macOS first:** primary build script `scripts/build_macos.sh`, docs and test plan updated
- Windows remains supported via `scripts/build_windows.bat` (secondary)

## [Unreleased] — M5: Polish & Host Cert
- Performance profiling
- Multi-instance testing
- Transport edge case hardening
- Packaging for distribution
