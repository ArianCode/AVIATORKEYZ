# AviatorKeyz — Changelog

All notable changes to this project will be documented here.
Format: [Version] — Date — Summary

---

## [Unreleased] — 2026-09-13 — Sound tabs: Bass / Keys / Plucks, new order, re-filed bank

### Changed
- **Browser tab order** is now Bass, Leads, Keys, Brass, Phrases, Arps, Synths,
  Bells, Strings, Plucks, Ensembles, Pads, Vocals. Phrases remains the only
  STRETCH tab — speed and host sync change its length, never its key; every
  other tab is a chromatic instrument.
- **Three new tabs — Bass, Keys and Plucks** — filled from content that was
  filed in the wrong place rather than from new samples. 150 sounds moved:
  36 bass patches out of Brass (plus 2 electric basses out of Strings), 80 keys
  built from Pads (45), Leads (22), Bells (8) and Synths (5), 12 plucks out of
  Bells and Synths, 12 real brass sections recovered from Ensembles, and the
  Synths textures moved to Pads. Nothing was deleted.
- **Bass loads monophonic.** A new note chokes the previous one through the
  engine's short crossfade, so overlapping sub tails can't stack into a muddy
  low end. Play Mode is still yours to change; leaving Bass only clears the
  Mono this rule imposed and leaves a deliberate Legato alone.
- `ContentImport/Chords` renamed to `ContentImport/Phrases` — the tab was
  renamed long ago and `import_factory_bank.py` had been rejecting the folder.

### Added
- `Scripts/refile_factory_categories.py` — classifies factory samples by name,
  renames the WAVs, rewrites each preset's `category`/`sampleId`, and mirrors
  the moves into `ContentImport/`. Dry run by default; `--apply` to commit.

### Fixed
- **Sessions saved before a re-file no longer fall back to the default sample.**
  `FactoryResources::findWavResource` now retries on the sample's stem when the
  `factory_<category>_` prefix no longer matches, so an old `factory_pads_…` id
  still resolves after the sound moved to Keys.

---

## [Unreleased] — 2026-09-13 — Noise-floor fixes, Grain Cloud repair, CHOP FADE

### Fixed
- **Delay hissed into an idle mix.** TAPE and ANALOG injected noise every sample
  whether or not anything was playing: measured −87 dBFS (tape) and −69 dBFS
  (analog, and unfiltered white) with a silent input. Noise now sits behind a
  signal-presence follower (fast open, 600 ms close) and analog's is low-passed
  into circuit hiss rather than white noise. Silent input now measures −240 dB.
- **PITCH mode hash.** The rotating-head shifter's artefacts compounded on every
  pass through the feedback loop. A 7 kHz roll-off in the pitch path takes ~2.5 dB
  off the hash-to-tail ratio and darkens the repeats as they climb. PITCH is still
  the grainiest mode; a pitch-synchronous shifter is the real fix.
- **Grain Cloud produced only white noise.** Two bugs: the MFX wrapper passed a
  fixed `0.55` grain rate into a mapping that read it as a *division*, yielding
  ~0.4 grains/second (roughly one 80 ms grain every 10 s at normal mix), and the
  "air" input was a literal white-noise generator fed from the SMEAR knob. Grain
  rate now comes from DENSITY (6..100 grains/s, capped to the voice pool), levels
  are normalised for overlap so density changes thickness not loudness, and the
  noise generator is gone. Measured tail after the input stops: −240 dB → −41 dB,
  and it is tone, not noise (HF −113 dB).
- **Grain scan/motion.** `spawnGrain` overwrote the shared scan position, so
  MOTION did nothing and grains read from ~3 s of stale audio. Grains now read
  behind the write head (POSITION 0 = what you just played) and MOTION sweeps.

### Added
- **CHOP FADE** (`slice_xfade`, 0–50 ms, default 2 ms): a fade applied at every
  chop / slice / flip edge so clicky chops can be repaired by hand. Drag either
  top corner of the Cargo Hold waveform inward, DAW-clip style, or use the FADE
  chip in the control row. Clamped per voice to under half the slice so short
  slices still speak.

### Notes
- Grain density no longer scales with mix; a granular tail is now proportional to
  the mix and dies once the 4 s capture refills with silence (test updated).
- A hard note onset on a sample whose first frame is away from zero still clicks.
  Fixing that automatically contradicts "zero attack starts at full level"
  (EnvelopeTests), so it is left to CHOP FADE rather than forced on every note.

## [Unreleased] — 2026-09-10 — Aviation Delay: multi-model delay engine

### Added
- **Aviation Delay** (`Source/DSP/Mfx/AviationDelay.*`) replaces Tape Echo as MFX
  effect index 2, so saved slots still load a delay. Uses all 16 generic slots, no
  new parameter IDs: MODE, STYLE, TIME, SYNC (13 divisions), FEEDBACK, MIX,
  DIFFUSION, MOD DEPTH, MOD RATE, LO CUT, HI CUT, AGE, DUCK, PITCH, RATIO, WIDTH.
- MODE changes the delay line itself. All colouring sits inside the feedback loop,
  so each repeat is coloured again:
  CLEAN (Hermite fractional delay + chorus mod), TAPE (record saturation,
  delay-time-dependent gap loss, wow/flutter/drift, asperity noise, splice bumps
  and dropouts with AGE), ANALOG (asymmetric diode clip, drift, hiss), BBD
  (compander, clock-rate sample & hold with tracking anti-alias filters,
  signal-dependent noise), LO-FI (32 kHz/15-bit → 5.5 kHz/6-bit converter),
  PITCH (rotating-head shifter in the loop, L/R detune), REVERSE (windowed
  backwards grains), CLOUD (8-stage modulated diffusion, long-tail feedback).
- STYLE sets routing: SINGLE, STEREO, PING-PONG, DUAL (R = T·ratio), RATIO
  (snapped musical ratios, cross-fed), QUAD (taps at T·r³, T·r², T·r, T).
- DIFFUSION: all-pass chain in the record path. Its nominal delay is subtracted
  from the playback head, and it fades in over the first 5 % of the knob, so
  0 % stays an exact, uncoloured delay.
- DUCK: one-knob, program-dependent. The dry signal holds the wet down; the
  wet blooms when you stop playing.
- 8 factory presets (one per mode); `Mfx::kMaxPresets` 4 → 8.
- Tests: 7 new MFX cases (echo timing, ping-pong, diffusion smear, octave pitch,
  ducking, 48 mode×style runs at 100 % feedback, click-free mode switching).
  `AVIATORKEYZ_DELAY_DEMO_DIR=<dir>` renders each preset to WAV.
  Debug snapshot harness: `AVIATORKEYZ_SNAPSHOT_MFX_A=<effect>[:<preset>]`.


## [Unreleased] — 2026-09-05 (evening) — Time stretch, slice pads, flipper controls, front-page pan + envelope switch

### Added
- **STRETCH playback mode** (`Source/DSP/StretchPlayer.*`, Signalsmith Stretch
  MIT, vendored under `Source/ThirdParty/`): pitch-preserving 0.25x–4x via
  `src_speed`, host-BPM sync on top, KEY TRACK becomes a transpose so a phrase
  plays chromatically at constant length. Selected with the Cargo Hold STRETCH
  segment; the flip lever reverses the stretched stream in place.
- **SLICE keyboard mode** ("mini sampler"): in SLICE playback mode every key is
  a pad — C1 = slice 1 of 16 across the trimmed window, polyphonic, gated, at
  the recorded pitch. The Cargo Hold waveform shows the 16 pad boundaries.
- Flip lever: `flip_mode` (LATCH / MOMENTARY), `flip_trigger_on` +
  `flip_trigger_note` (a MIDI note throws the lever and is consumed, default
  C0), and five FLIP PRESETS (window / snap / mode combos).
- MAIN page: the blueprint aircraft is now the PAN control — an outline plane
  cruises left → right, the filled plane sits at the pan position and can be
  dragged (double-click = centre).
- MAIN page ENVELOPE: `env_enabled` master switch (title click, LED) and a ↺
  reset glyph that returns A/D/S/R to defaults. OFF = instant attack, full
  sustain, short release.
- Cargo Hold SPEED chip (drag; click = 1.00x), labelled STRETCH in stretch mode.
- Schema: +4 IDs (`flip_mode`, `flip_trigger_on`, `flip_trigger_note`,
  `env_enabled`) → 322 total. Tests: 187 (+StretchAndSliceTests).

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
