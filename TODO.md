# AviatorKeyz — TODO

Priority labels: [P0] = blocking, [P1] = current milestone, [P2] = next milestone, [P3] = backlog

**Last updated:** 2026-08-19

---

## M1 — Core Sampler Engine [~90%]

- [x] SampleMap / SampleLibrary double-buffer
- [x] SamplerEngine voice pool + steal-quietest
- [x] MidiHandler note/CC/sustain/all-notes-off
- [x] Glide, reverse, ADSR wired
- [ ] [P1] Land MIDI keytrack (`keytrack` drives pitch) + `PitchTrackingTests`
- [ ] [P1] Host exit tests T-M1-01–08 (Standalone + DAW)

---

## M2 — Creative Engine [~85%]

- [x] ToneShaper, SmearProcessor, ReverbTail, width/pan
- [x] SynthEngine + source blend + mod matrix
- [x] PHRASE_KEY_SYNC implemented in SamplerEngine
- [ ] [P1] Automation stress T-M2-10 (no zipper noise)

---

## M3 — Preset System [~80%]

- [x] Per-preset factory bank embedded via BinaryData (114 XML / 111 WAV)
- [x] PresetManager load/save + MAIN preset dropdown
- [x] Host state restore (sampleId + preset identity in project state)
- [ ] [P2] Schema migration when STATE_SCHEMA_VERSION bumps
- [ ] [P0] Factory content freeze + sample clearance (checklist still 11-WAV model)

---

## M4 — Premium UI [~80%]

- [x] Aviation MAIN cockpit (`Source/GUI/Aviation/`, Jul 20)
- [x] Preset dropdown, macro deck, glass panels, status bar
- [x] Advanced / PERFORMANCE view (synth/mod/FX, KEY TRACK)
- [ ] [P1] Full LuxuryLookAndFeel (ComboBox, ListBox)
- [ ] [P2] Waveform display on cockpit photo
- [ ] [P3] Animated windshield / legacy UI polish

---

## M5 — Polish & Host Cert [~15%]

- [x] Automated smoke script (`scripts/run_m5_smoke.sh`)
- [x] M5 checklist (`docs/M5_CERTIFICATION.md`)
- [x] Notarization template (`scripts/notarize_macos.sh`)
- [ ] [P1] Multi-instance test T-M5-01
- [ ] [P1] CPU profiling T-M5-04
- [ ] [P2] 48h soak T-M5-05
- [ ] [P1] Release codesign + notarize (requires Apple Developer ID)
- [ ] [P1] Golden DAW host smoke (Logic / Ableton / Reaper)

---

## Ongoing / Housekeeping

- [x] GitHub remote + initial push
- [x] C++ + Python unit test suites
- [x] Factory import pipeline (`scripts/import_factory_bank.py`)
- [ ] [P2] CI pipeline (GitHub Actions macOS build on push)
- [ ] [P3] User sample drag-import UI
