# AviatorKeyz — Changelog

All notable changes to this project will be documented here.
Format: [Version] — Date — Summary

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
