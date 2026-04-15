# AviatorKeyz — TODO

Priority labels: [P0] = blocking, [P1] = current milestone, [P2] = next milestone, [P3] = backlog

---

## M1 — Core Sampler Engine [CURRENT]

- [P0] Design `SampleMap` data structure (note → buffer mapping, velocity layers)
- [P0] Implement `SamplerVoice` — read position, playback state, velocity gain
- [P0] Implement fixed voice pool with "steal quietest" strategy in `SamplerEngine`
- [P0] Wire `MidiHandler` → `SamplerEngine` note-on / note-off
- [P0] Handle `all-notes-off` (CC123) and `all-sound-off` (CC120)
- [P1] Sustain pedal hold logic (notes continue after key release until pedal released)
- [P1] Velocity-to-amplitude curve (linear or slight curve)
- [P1] Pitch ratio calculation: `pow(2, (note - rootNote) / 12.0f)`
- [P1] Wire GlideEngine into SamplerEngine pitch ratio per voice
- [P1] Implement `prepareToPlay` / `releaseResources` lifecycle in SamplerEngine
- [P1] Test: FL Studio scan (T-M0-03), plugin loads (T-M0-04)
- [P2] Double-buffer for sample map swaps (message thread writes, audio thread reads)
- [P2] Support loading .wav and .aif files via juce::AudioFormatManager

---

## M2 — Creative Engine [P2]

- [P1] ToneShaper: implement two cascaded IIR shelving filters
- [P1] ToneShaper: coefficient update every 64 samples from smoothed tone value
- [P1] SmearProcessor: implement convolution kernel or multi-tap diffusion
- [P1] ReversePlayer: integrate into SamplerVoice read-position logic
- [P1] GlideEngine: integrate tick() call into SamplerEngine per-sample loop
- [P2] Validate Reverse + Glide interaction (independent, no special handling needed)
- [P2] Validate no zipper noise on all 4 creative controls under automation (T-M2-10)

---

## M3 — Preset System [P2]

- [P1] Binary-embed factory preset XMLs via `juce_add_binary_data`
- [P1] PresetManager: implement `getPresetsForCategory()` from factory + user dirs
- [P1] PresetManager: implement `loadPreset()` with schema version check
- [P1] PresetManager: implement `saveUserPreset()` to user Documents dir
- [P1] PresetBrowser: category sidebar (10 categories), scrollable preset list
- [P2] Write 5+ factory presets per category (50+ total)
- [P2] Preset migration test: load a v1 preset after STATE_SCHEMA_VERSION bump

---

## M4 — Premium UI [P2]

- [P1] LuxuryLookAndFeel: `drawRotarySlider` — arc track, gold pointer, glow
- [P1] LuxuryLookAndFeel: `drawButtonBackground` — pill shape, gold active state
- [P1] LuxuryLookAndFeel: `drawComboBox` — minimal, gold caret
- [P1] MainPanel: full layout — header, waveform, knob row, space section
- [P1] HeaderBar: logo image, preset name display, nav tab buttons
- [P1] WaveformDisplay: render `juce::AudioThumbnail` of loaded sample
- [P1] KnobComponent: double-click to reset to default
- [P1] KnobComponent: right-click context menu (reset, enter value)
- [P2] Editor resize: all elements scale at fixed aspect ratio
- [P2] Waveform: highlight triggered note zone on note-on (lock-free signal from DSP)

---

## M5 — Polish & FL Studio Cert [P3]

- [P1] Multi-instance test: 4 simultaneous instances (T-M5-01)
- [P1] Transport start/stop edge case testing (T-M5-02)
- [P1] Plugin bypass test (T-M5-03)
- [P1] CPU profiling — target <5% at 32 voices, 44.1 kHz, 256 buf (T-M5-04)
- [P2] 48h soak test for memory leaks (T-M5-05)
- [P2] Release build packaging — signed .vst3 bundle
- [P3] User sample import UI (drag-and-drop onto waveform display)
- [P3] Sample library browser (multi-file sample map assignment)
- [P3] macOS build support

---

## Ongoing / Housekeeping

- [P1] Initialize Git repo with initial commit after M0 scaffold
- [P2] Add juce::UnitTest subclasses for GlideEngine, StateSchema
- [P3] CI pipeline (GitHub Actions: Windows CMake build on push)
