# AviatorKeyz — Master TODO

Priority: [P0] blocking | [P1] current milestone | [P2] next milestone | [P3] backlog

---

## M0 EXIT CRITERIA (must all pass before M1 begins)

- [ ] [P0] Build succeeds: `./scripts/build_macos.sh` produces AviatorKeyz.vst3 with no errors
- [ ] [P0] Standalone launches without crash
- [ ] [P0] macOS DAW scan: plugin appears as VST3 instrument (not effect)
- [ ] [P0] Plugin window opens without crash
- [ ] [P0] Window resizes within bounds (min 700×404, max 1800×1040)
- [ ] [P0] Project save → close → reopen → plugin loads at correct defaults

---

## M1 — Core Sampler Engine

### Data structures
- [ ] [P1] Define `SampleMap` struct: array of SampleRegion (noteMin, noteMax, rootNote, velMin, velMax, bufferPtr)
- [ ] [P1] Define `SamplerVoice` struct: readPosition, isPlaying, noteNumber, velocityGain, pitchRatio, reversed

### SamplerEngine
- [ ] [P1] Implement fixed 64-voice pool in `SamplerEngine`
- [ ] [P1] Voice allocation: find first free voice; if none, steal quietest
- [ ] [P1] `noteOn(int note, float velocity, bool reversed, float glideTimeMs)`
- [ ] [P1] `noteOff(int note)`
- [ ] [P1] `allNotesOff()` / `allSoundOff()`
- [ ] [P1] Per-voice pitch ratio: `pow(2.0f, (note - rootNote) / 12.0f)`
- [ ] [P1] Per-voice read loop in `process()`: advance readPosition, mix into output buffer
- [ ] [P1] Integrate GlideEngine.tick() inside per-sample voice loop

### MidiHandler
- [ ] [P1] Wire note-on → SamplerEngine.noteOn()
- [ ] [P1] Wire note-off → SamplerEngine.noteOff()
- [ ] [P1] Sustain pedal: hold active voices until pedal release
- [ ] [P1] CC123 (all notes off) → SamplerEngine.allNotesOff()
- [ ] [P1] CC120 (all sound off) → SamplerEngine.allSoundOff()

### Sample loading
- [ ] [P1] Add `juce::AudioFormatManager` + `juce::AudioFormatReaderSource` to SampleLibrary
- [ ] [P1] Load .wav and .aiff files from disk into `juce::AudioBuffer<float>`
- [ ] [P1] Implement double-buffer swap for SampleMap updates (message thread write, audio thread read)

### Testing
- [ ] [P1] Verify T-M1-01 through T-M1-08 (see TESTPLAN.md)

---

## M2 — Creative Engine

- [ ] [P1] ToneShaper: two cascaded IIR shelf filters, coefficient update every 64 samples
- [ ] [P1] SmearProcessor: benchmark convolution kernel vs. multi-tap diffusion; implement winner
- [ ] [P1] ReversePlayer: integrate into SamplerVoice read loop (read decrement + boundary clamp)
- [ ] [P1] GlideEngine: verify ramp quality at all glide_time values under automation
- [ ] [P1] ReverbTail: wire juce::dsp::Reverb to reverb_amount + reverb_size params
- [ ] [P1] Stereo width: implement M-S matrix (Mid = L+R, Side = L-R; width scales Side)
- [ ] [P2] Crossfade at Reverse boundary (10 ms default to mask cut)
- [ ] [P2] Test T-M2-01 through T-M2-10

---

## M3 — Preset System

- [ ] [P1] Binary-embed factory XMLs via `juce_add_binary_data`
- [ ] [P1] PresetManager: `getPresetsForCategory()` from embedded factory + user dir
- [ ] [P1] PresetManager: `loadPreset()` with schema version check
- [ ] [P1] PresetManager: `saveUserPreset()` to `~/Documents/AviatorKeyz/Presets/`
- [ ] [P1] PresetBrowser: category sidebar (10 items), scrollable preset list, Save button
- [ ] [P2] Author 5+ factory presets per category (50+ total)
- [ ] [P2] Preset migration: test loading a v1 preset after STATE_SCHEMA_VERSION bump

---

## M4 — Premium UI

- [ ] [P1] LuxuryLookAndFeel: `drawRotarySlider` — arc track, gold pointer, glow on hover
- [ ] [P1] LuxuryLookAndFeel: `drawButtonBackground` — pill shape, gold when active
- [ ] [P1] LuxuryLookAndFeel: `drawComboBox` — minimal, gold caret
- [ ] [P1] MainPanel: complete layout (header, waveform strip, knob row, space section)
- [ ] [P1] HeaderBar: logo asset, preset name display, tab buttons
- [ ] [P1] WaveformDisplay: juce::AudioThumbnail rendering + note-trigger highlight
- [ ] [P1] KnobComponent: double-click resets to default
- [ ] [P1] KnobComponent: right-click context menu (Reset to default / Enter value)
- [ ] [P2] Proportional resize at all sizes

---

## M5 — Polish & Host Certification (macOS)

- [ ] [P1] Multi-instance test: 4 simultaneous instances, no shared state
- [ ] [P1] Transport start/stop: verify no stuck notes on loop restart
- [ ] [P1] Plugin bypass test
- [ ] [P1] CPU profile: <5% at 32 voices, 44.1 kHz, 256 buf
- [ ] [P2] 48h soak test
- [ ] [P2] Release packaging: codesign + notarize .vst3 (macOS)

---

## Ongoing

- [ ] [P1] Init git repo with first commit (M0 scaffold)
- [ ] [P2] Unit tests for GlideEngine, StateSchema, SamplerVoice pitch ratio
- [ ] [P3] CI: GitHub Actions macOS build on push
- [ ] [P3] Windows secondary build + FL Studio scan
- [ ] [P3] User sample import UI (drag-and-drop)
