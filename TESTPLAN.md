# AviatorKeyz — Test Plan

**Format:** Manual test cases (automated unit tests added in M2+)

---

## M0 — Scaffold & Build

### T-M0-01: Build succeeds (Release, macOS)
- Run `./scripts/build_macos.sh`
- Expected: no errors, `AviatorKeyz.vst3` present in build output

### T-M0-02: Build succeeds (Debug, macOS)
- Run `./scripts/build_macos.sh debug`
- Expected: debug build with `AVIATORKEYZ_DEBUG=1` defined

### T-M0-03: Standalone launches (macOS)
- Run `open build/AviatorKeyz_artefacts/Standalone/AviatorKeyz.app` (Ninja paths; add `/Release` if using Xcode)
- Expected: window opens, dark background + gold UI visible, no crash

### T-M0-04: DAW plugin scan (macOS)
- Copy VST3 to `~/Library/Audio/Plug-Ins/VST3/`
- Rescan in host (Logic, Ableton, Reaper, or FL Studio for Mac)
- Expected: "AviatorKeyz" appears as a **VST3 instrument**, not an effect

### T-M0-05: Plugin loads without crash
- Insert AviatorKeyz on an instrument track
- Expected: plugin window opens, no crash, no error dialog

### T-M0-06: Resizing
- Drag the plugin window to resize
- Expected: window resizes smoothly, constrains to min (700×404) and max (1800×1040)

### T-M0-07: Project save/reload
- Open plugin, save host project, quit host, reopen project
- Expected: plugin loads with same state (all params at defaults)

### T-M0-W01: Windows build (secondary)
- Run `scripts\build_windows.bat`
- Expected: Release VST3 builds on Windows 10/11 x64

---

## M1 — Core Sampler Engine

### T-M1-01: MIDI note triggers sample
- Load a test sample, play MIDI note C4
- Expected: sample plays at correct pitch

### T-M1-02: Polyphony
- Hold 8 simultaneous notes
- Expected: all 8 play without voice stealing

### T-M1-03: Voice stealing
- Hold 65 simultaneous notes (exceeds pool of 64)
- Expected: quietest voice stolen, no crash

### T-M1-04: Note-off
- Trigger note, release key
- Expected: sample stops (or enters release phase)

### T-M1-05: Sustain pedal
- Press sustain, release key — notes should continue
- Release sustain — notes stop

### T-M1-06: All notes off (MIDI CC123)
- Send CC123 — all active voices should stop

### T-M1-07: Sample rate change
- Change host project sample rate (44100 → 48000 → 96000)
- Expected: plugin prepares correctly, no pitch shift, no crash

### T-M1-08: Buffer size change
- Change FL audio buffer size (256 → 512 → 1024 samples)
- Expected: stable playback, no glitches

---

## M2 — Creative Engine

### T-M2-01: Reverse — basic
- Enable Reverse, trigger note
- Expected: sample plays backward

### T-M2-02: Reverse — MIDI timing preserved
- Play 4-note phrase with Reverse on
- Expected: each note's rhythmic position matches non-reversed version

### T-M2-03: Glide — basic
- Set Glide to 100 ms, play two notes
- Expected: audible pitch slide between notes

### T-M2-04: Glide — at 0 ms
- Set Glide to 0 ms
- Expected: instant pitch jump, no slide

### T-M2-05: Smear — at 0.0
- Set Smear to 0 — expected: no audible effect, sharp transients

### T-M2-06: Smear — at 1.0
- Set Smear to 1.0 — expected: transients blurred, washed texture

### T-M2-07: Tone — neutral
- Tone at 0.0 — expected: flat frequency response (bypass)

### T-M2-08: Tone — bright
- Tone at +1.0 — measure: high frequencies boosted relative to neutral

### T-M2-09: Tone — dark
- Tone at -1.0 — measure: high frequencies reduced, warmth added

### T-M2-10: No zipper noise
- Automate all creative controls (Reverse, Glide, Smear, Tone) during playback
- Expected: no audible stepping or clicks

---

## M3 — Preset System

### T-M3-01: Factory presets load
- Open preset browser, select each category
- Expected: factory presets listed, load without error

### T-M3-02: Preset recall — all parameters
- Load preset, verify all 9 parameter values match preset file

### T-M3-03: User preset save
- Modify parameters, save as user preset
- Expected: file created in ~/Documents/AviatorKeyz/Presets/

### T-M3-04: Project save/recall with custom preset
- Load user preset, save FL project, close FL, reopen
- Expected: same preset state recalled correctly

### T-M3-05: Preset browser categories
- All 10 categories visible: Leads, Brass, Ensembles, Strings, Pads, Phrases, Synths, Arps, Vocals, Bells

---

## M4 — Premium UI

### T-M4-01: Knobs render correctly
- All 7 knobs display with gold accent, label, value tooltip on hover

### T-M4-02: Waveform display
- Load sample — waveform renders in center panel

### T-M4-03: Resize — layout scales
- Resize from minimum to maximum — all elements scale proportionally

### T-M4-04: No UI artifacts under automation
- Automate multiple params simultaneously — UI updates smoothly without flicker

---

## Post-DSP Wiring — Host Smoke (run after unit tests pass)

**Primary FL Studio test host:** FL Studio 2025 **25.2.3.4889**, ARM64 native, macOS 15.7.4 — [docs/HOST_TEST_CONFIG.md](docs/HOST_TEST_CONFIG.md)

Run automated checks first:

```bash
cmake --build build --target AviatorKeyzTests && \
  ./build/tests/AviatorKeyzTests_artefacts/Release/AviatorKeyzTests
python3 -m unittest tests/test_state_schema.py -v
cp -R "build/AviatorKeyz_artefacts/Release/VST3/AviatorKeyz.vst3" "$HOME/Library/Audio/Plug-Ins/VST3/"
```

### T-DSP-01: FL Studio scan + note-on
- Rescan plugins in FL Studio **25.2.3.4889**; load AviatorKeyz on an instrument track
- Play one MIDI note while monitoring CPU
- Expected: audio output, no crash, no CPU spike on note-on

### T-DSP-02: Preset change under playback
- While holding a note or looping MIDI, change factory presets across categories
- Expected: no crash, sample reloads, playback continues

### T-DSP-03: `source_blend` automation stress
- Automate **Source Blend** (`source_blend`) from 0 → 1 over 2 bars while audio plays
- Expected: smooth crossfade between sample and synth; no zipper noise, pops, or hang

---

## M5 — Polish & Host Certification

### T-M5-01: Multiple instances (macOS)
- Open 4 simultaneous instances in your primary DAW
- Expected: all instances work independently, no shared state conflicts

### T-M5-02: Transport start/stop
- Start/stop host transport repeatedly — plugin responds correctly

### T-M5-03: Plugin bypass
- Enable/disable plugin bypass in host
- Expected: signal passes cleanly when bypassed, no artifacts on toggle

### T-M5-04: CPU profiling
- 32-voice polyphony at 44.1 kHz, 256 samples buffer
- Expected: CPU usage < 5% on modern hardware

### T-M5-05: 48h soak test
- Loop MIDI sequence for 48 hours
- Expected: no memory leak, no crash, no drift
