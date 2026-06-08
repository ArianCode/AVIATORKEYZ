# AviatorKeyz — TODO

Priority labels: [P0] = blocking, [P1] = current milestone, [P2] = next milestone, [P3] = backlog

**Last updated:** 2026-06-05

---

## M1 — Core Sampler Engine [~90%]

- [x] SampleMap / SampleLibrary double-buffer
- [x] SamplerEngine voice pool + steal-quietest
- [x] MidiHandler note/CC/sustain/all-notes-off
- [x] Glide, reverse, ADSR wired
- [ ] [P1] Host exit tests T-M1-01–08 (Standalone + DAW)

---

## M2 — Creative Engine [~85%]

- [x] ToneShaper, SmearProcessor, ReverbTail, width/pan
- [x] SynthEngine + source blend + mod matrix
- [x] PHRASE_KEY_SYNC implemented in SamplerEngine
- [ ] [P1] Automation stress T-M2-10 (no zipper noise)

---

## M3 — Preset System [~80%]

- [x] ~110 per-preset factory WAVs + XML embedded via BinaryData
- [x] PresetManager load/save + PresetBrowser UI
- [x] Host state restore (sampleId + preset identity in project state)
- [ ] [P2] Schema migration when STATE_SCHEMA_VERSION bumps

---

## M4 — Premium UI [~60%]

- [x] CockpitCrossworldPanel + aviation gauge bar
- [x] Photo-anchored knobs/toggles (CockpitZones → PhotoAnchoredKnob)
- [x] Advanced view (synth/mod/FX)
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

---

## Ongoing / Housekeeping

- [x] GitHub remote + initial push
- [x] C++ + Python unit test suites
- [x] Factory import pipeline (`scripts/import_factory_bank.py`)
- [ ] [P2] CI pipeline (GitHub Actions macOS build on push)
- [ ] [P3] User sample drag-import UI
