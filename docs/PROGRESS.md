# AviatorKeyz — Progress tracker

**Last updated:** 2026-05-19  
**Branch:** `main` (M1 checkpoint committed; bypass + tests in working tree)

Legend: `█` done · `░` remaining

---

## Milestone bars

```
M0  Scaffold & build          ████████████  100%  ✅ committed
M1  Sampler + MIDI            ██████████░░   85%  🔧 in progress
M2  Creative DSP              ████████░░░░   70%  wired; tune & validate
M3  Presets                    ████░░░░░░░░   25%  2 factory + browser UI
M4  Premium UI                 ██████░░░░░░   50%  layout + L&F; mockup polish TBD
M5  Host certification (macOS) ░░░░░░░░░░░░    0%  not started
```

| # | Milestone | % | State |
|---|-----------|---|--------|
| M0 | Scaffold & build | 100% | Done (committed) |
| M1 | Core sampler + MIDI | 85% | **Current focus** |
| M2 | Creative engine | 70% | Ahead of schedule in tree; finish M1 first |
| M3 | Preset system | 25% | — |
| M4 | Premium UI | 50% | — |
| M5 | Host cert (macOS) | 0% | — |

---

## M1 — What’s left (finish checklist)

See [MILESTONES.md](MILESTONES.md) for full exit criteria.

| Task | Status |
|------|--------|
| Fix build (`createReaderFor` / JUCE 8) | ✅ |
| Connect `SampleLibrary` → `SamplerEngine` (note + velocity → right sample) | ✅ |
| Double-buffer sample map (safe updates, no audio-thread file I/O) | ✅ |
| Bypass MIDI state drain (`processBlockBypassed`) | ✅ (working tree) |
| C++ unit tests synced to `AudioSnapshot` API | ✅ (working tree) |
| Verify exit tests T-M1-01 … T-M1-08 in host (Standalone or macOS DAW) | ⬜ |
| Commit M1 work to git | ✅ checkpoint |

**Already in code (working tree):** 64 voices, steal-quietest, MIDI on/off, sustain, all-notes-off, glide, reverse, ADSR, factory WAV embed, basic sample playback.

---

## How to update this file

After each milestone push or major session, adjust the bars and `%` column. Bump **Last updated** date.
