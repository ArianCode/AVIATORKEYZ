# AviatorKeyz — Milestones

Each milestone must be fully tested before work on the next begins.
Exit criteria are the definition of "done" — not "mostly done."

---

## M0 — Scaffold & Build ✅ COMPLETE

**Goal:** Compilable JUCE VST3 instrument shell that scans correctly in FL Studio.

**Deliverables:**
- CMakeLists.txt with JUCE via FetchContent (pinned version, reproducible)
- PluginProcessor with APVTS + all 9 parameters declared
- PluginEditor with resizable placeholder UI (dark gold palette)
- StateSchema.h with all param IDs and schema version
- All DSP/GUI/MIDI/State module skeletons (safe, compilable stubs)
- docs/ folder with all spec documents
- build script for Windows

**Exit criteria:**
- [ ] `scripts/build_windows.bat` produces `AviatorKeyz.vst3` with no errors
- [ ] Plugin appears under Instruments in FL Studio plugin scan (not Effects)
- [ ] Plugin window opens without crash
- [ ] Plugin window resizes within set bounds
- [ ] Project save → close → reopen → plugin loads with correct default state

---

## M1 — Core Sampler Engine

**Goal:** MIDI triggers real sample playback with polyphony.

**Deliverables:**
- `SampleMap` data structure (note → sample buffer, root note, velocity range)
- `SamplerVoice` — read position, amplitude envelope (ADSR stub), velocity gain, pitch ratio
- `SamplerEngine` — fixed 64-voice pool with "steal quietest" strategy
- `MidiHandler` — note-on/off, sustain pedal, all-notes-off fully wired
- `GlideEngine` integrated into per-voice pitch calculation
- WAV / AIFF file loading via `juce::AudioFormatManager`
- Double-buffer for sample map swaps (message thread writes, audio thread reads)

**Exit criteria:**
- [ ] Play MIDI note → sample plays at correct pitch
- [ ] 8 simultaneous notes play without stealing
- [ ] Sustain pedal holds notes after key release
- [ ] All-notes-off stops all voices immediately
- [ ] No artifacts on sample rate change (44.1 → 48 → 96 kHz)
- [ ] No artifacts on buffer size change (256 → 512 → 1024 samples)

---

## M2 — Creative Engine

**Goal:** All four signature controls (Reverse, Glide, Smear, Tone) are functional and automatable with no zipper noise.

**Deliverables:**
- `ReversePlayer` integrated into SamplerVoice — backward read from buffer end
- `GlideEngine` — per-note ramp confirmed working with pitch ratio
- `SmearProcessor` — transient blur kernel (convolution or diffusion, profiling determines which)
- `ToneShaper` — two cascaded IIR shelving filters, coefficient refresh every 64 samples
- `ReverbTail` — JUCE `dsp::Reverb` wired to reverb_amount + reverb_size
- Stereo width M-S matrix wired to stereo_width param
- All smoothed params driving DSP without audible stepping

**Exit criteria:**
- [ ] Reverse: notes play backward; MIDI timing preserved
- [ ] Glide: audible pitch slide at 100 ms; instant at 0 ms
- [ ] Smear: 0.0 = clean transients; 1.0 = blurred wash
- [ ] Tone: +1.0 = measurably brighter; -1.0 = measurably darker
- [ ] All 4 controls automatable in FL Studio without clicks or zipper noise

---

## M3 — Preset System

**Goal:** Presets save, load, and recall correctly in FL Studio. Factory library populated.

**Deliverables:**
- Factory presets as XML files binary-embedded via `juce_add_binary_data`
- `PresetManager` — full enumerate / load / save
- `PresetBrowser` UI — category sidebar + scrollable preset list + Save button
- User preset save to `~/Documents/AviatorKeyz/Presets/`
- Preset format versioned; migration tested

**Exit criteria:**
- [ ] All 10 categories visible in browser
- [ ] 5+ factory presets per category (50+ total)
- [ ] Load preset → all 9 param values match preset file
- [ ] Save user preset → file appears in user docs folder
- [ ] FL Studio project save → close → reopen → preset state recalled exactly

---

## M4 — Premium UI

**Goal:** Plugin looks and feels like a luxury instrument. Matches reference UI mockup.

**Deliverables:**
- `LuxuryLookAndFeel` — full custom rotary knob, button, combobox draw
- `MainPanel` — complete layout (header, waveform, knob row, space section)
- `HeaderBar` — logo, active preset name, nav tabs
- `WaveformDisplay` — sample thumbnail rendering
- `KnobComponent` — double-click reset, right-click context menu
- Smooth resize scaling at all sizes from min to max

**Exit criteria:**
- [ ] UI matches reference mockup direction (gold/dark, minimal, premium)
- [ ] All knobs show label + value tooltip on hover
- [ ] Waveform renders for loaded sample
- [ ] Resize: all elements scale cleanly, no clipping
- [ ] No UI artifacts or flicker under simultaneous automation

---

## M5 — Polish & FL Studio Certification

**Goal:** Plugin ships. No known crashes. FL Studio sign-off on all major behaviors.

**Deliverables:**
- Multi-instance test (4 simultaneous instances)
- Transport start/stop edge case resolution
- Plugin bypass behavior verified
- CPU profiling — target <5% at 32 voices, 44.1 kHz, 256 buf
- 48-hour soak test (no memory leak, no crash, no drift)
- Release build signed and packaged as `.vst3` bundle

**Exit criteria:**
- [ ] All test cases in `TESTPLAN.md` passing
- [ ] 48h soak test completed without crash or leak
- [ ] CPU <5% at 32 voices
- [ ] Signed VST3 bundle installs cleanly on a fresh Windows machine

---

## Rule

No milestone is considered complete until all its exit criteria checkboxes are checked.
Work on M(n+1) does not begin until M(n) is complete.
