# AviatorKeyz — Progress tracker

**Last updated:** 2026-08-19  
**Branch:** `phase1-stabilize`

Legend: `█` done · `░` remaining

---

## Milestone bars

```
M0  Scaffold & build          ████████████  100%  ✅
M1  Sampler + MIDI            ██████████░░   90%  DAW sign-off open; keytrack WIP
M2  Creative DSP              █████████░░░   85%  automation stress TBD
M3  Presets                   █████████░░░   80%  114 XML / 111 WAV; clearance open
M4  Premium UI                ██████████░░   80%  Aviation MAIN landed Jul 20
M5  Host certification (macOS) ██░░░░░░░░░░   15%  scripts exist; manual DAW QA open
```

| # | Milestone | % | State |
|---|-----------|---|--------|
| M0 | Scaffold & build | 100% | Done |
| M1 | Core sampler + MIDI | 90% | Engine + unit tests strong; formal host sign-off open; MIDI keytrack uncommitted |
| M2 | Creative engine | 85% | Wired; zipper/automation validation open |
| M3 | Preset system | 80% | Per-preset factory bank (114 XML / 111 WAV); session restore; content freeze + clearance open |
| M4 | Premium UI | 80% | Aviation MAIN cockpit landed (Jul 20); Advanced/PERFORMANCE exists; polish + host visual QA remain |
| M5 | Host cert (macOS) | 15% | Checklist + smoke/notarize scripts; multi-instance, CPU, soak, codesign, DAW matrix open |

---

## Recent session (2026-08-19)

Status snapshot (docs refresh). Last git commit on this branch: 2026-07-20 (`e01e9d0` — Aviation MAIN docs).

**Landed in git (Jun–Jul):**
- Factory bank: per-preset WAVs + XML (now **114** factory XML / **111** embedded WAVs)
- Sample region is pitch/tempo source; root note / inferred BPM sync into APVTS on load
- Phase 1 DSP hygiene: de-click, no audio-thread allocs, pan law
- Aviation MAIN UI: cockpit canvas, preset dropdown, macro deck, glass panels, status bar
- PERFORMANCE tab = Advanced panel (macros, LFO, mod matrix, FX)

**Uncommitted WIP — MIDI keytrack:**
- `keytrack` is the runtime MIDI-pitch switch (not playback-mode-only)
- Ensembles one-shots (e.g. Electric Guitar Fading) → chromatic + keytrack
- Advanced KEY TRACK control; `PitchTrackingTests.cpp` not yet in git

**Not shippable yet:** factory clearance still treats WAVs as placeholders; M5 host/signing QA open. See root `TODO.md`.

---

## How to update this file

After each milestone push or major session, adjust the bars and `%` column. Bump **Last updated** date.
