# AviatorKeyz — Progress tracker

**Last updated:** 2026-06-05  
**Branch:** `main`

Legend: `█` done · `░` remaining

---

## Milestone bars

```
M0  Scaffold & build          ████████████  100%  ✅
M1  Sampler + MIDI            ██████████░░   90%  host exit tests pending
M2  Creative DSP              █████████░░░   85%  automation stress TBD
M3  Presets                    █████████░░░   80%  ~110 factory presets embedded
M4  Premium UI                 ███████░░░░░   60%  photo anchors wired
M5  Host certification (macOS) ██░░░░░░░░░░   15%  automated smoke script added
```

| # | Milestone | % | State |
|---|-----------|---|--------|
| M0 | Scaffold & build | 100% | Done |
| M1 | Core sampler + MIDI | 90% | Engine complete; formal host sign-off open |
| M2 | Creative engine | 85% | Wired; zipper/automation validation open |
| M3 | Preset system | 80% | Per-preset factory bank; session restore fixed |
| M4 | Premium UI | 60% | Cockpit + photo-anchored controls |
| M5 | Host cert (macOS) | 15% | Checklist + scripts; manual DAW QA open |

---

## Recent session (2026-06-05)

- Migrated factory bank: 11 monolithic WAVs → ~110 per-preset WAVs + XMLs
- Synced Python/C++ tests for new content model
- Host state restore: sampleId + preset identity saved in project state
- Wired `CockpitZones` photo-anchored knobs/toggles onto cockpit photo
- Added `scripts/run_m5_smoke.sh`, `docs/M5_CERTIFICATION.md`, `scripts/notarize_macos.sh`

---

## How to update this file

After each milestone push or major session, adjust the bars and `%` column. Bump **Last updated** date.
