# Prototype #1 — FL Studio validation matrix

Run **independently** on a **clean Windows machine** (no VS, no source tree, no prior Aviation install).  
Create a **native empty project** in each FL version — do **not** open cross-version `.flp` files (e.g. FL 25 Mac → FL 11 Windows, or FL 25 → FL 20).

**Target env:** [PROTOTYPE1_TARGET_ENV.md](PROTOTYPE1_TARGET_ENV.md)  
**Checklist copy for package:** [release/prototype1/TEST/TEST_CHECKLIST.txt](../release/prototype1/TEST/TEST_CHECKLIST.txt)

One **universal** `Aviation.vst3` bundle serves every column. 64-bit FL hosts (11.1+, 20,
21+, 25) load `Contents\x86_64-win`; FL 11 32-bit loads `Contents\x86-win`. Install the same
bundle to both VST3 paths — see [PROTOTYPE1_TARGET_ENV.md](PROTOTYPE1_TARGET_ENV.md).

**Record FL 11's bitness in the notes column** — it decides which half of the bundle that
column actually exercised. A `PASS` on FL 11 means nothing if you don't know which DLL loaded.

Intel vs AMD and Windows 10 vs 11 are **not** separate builds and do not need separate
columns. If you want coverage there, vary the machines you run the existing columns on and
note the CPU and OS build per run.

Legend: `PASS` / `FAIL` / `NOT TESTED` / `N/A`

| Test | FL 11 | FL 20 | FL 21+ | FL 25 | Notes |
|------|:-----:|:-----:|:------:|:-----:|-------|
| Plugin discovered | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | record FL bitness |
| Correct arch half loaded | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | x86-win vs x86_64-win |
| Classified as instrument | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | |
| UI opens | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | |
| UI closes/reopens 10× | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | |
| Correct factory preset loads | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | |
| Different MIDI notes pitch correctly | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | |
| Key Track | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | |
| Note-off terminates voice | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | |
| Fast overlapping bass notes | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | Use `Prototype_Test.mid` |
| Sustain pedal | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | |
| Rapid preset changes | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | |
| Reverse | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | |
| Glide | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | |
| Tone / macros | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | |
| Transport start/stop | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | |
| Panic / all-notes-off | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | |
| 2 instances | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | |
| 4 instances | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | |
| Save `.flp` (native project) | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | |
| Restart FL | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | |
| Restore `.flp` | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | |
| Parameter state recalled | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | |
| Preset recalled | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | |
| Sound recalled, not a sine | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | FLSI-008 regression |
| Render/export matches live | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | FLSI-008 regression |
| Consolidate/bounce matches live | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | FLSI-008 regression |
| 2+ instances keep separate presets | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | different preset per channel |
| 2+ instances survive one bounce | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | all instances release/prepare together |
| User preset saved then recalled | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | not just factory presets |
| 44.1 kHz | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | |
| 48 kHz | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | |
| 128-sample buffer | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | |
| 512 / 1024 buffer | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | |
| 125–150% Windows scaling | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | |

**FL 21+ column:** Record the exact version tested (e.g. FL 21.2, FL 24.1) in the release record when filling results.

---

## Failure classification

When logging issues in [known-host-issues.md](../known-host-issues.md), tag each entry:

- **FL11** — legacy wrapper / host-specific
- **FL20** — host-specific
- **FL21+** — newer 64-bit host (record exact version)
- **FL25** — current-generation host (Windows; record exact build)
- **packaging** — missing files, wrong path, ZIP layout
- **validator** — Steinberg VST3 validator failure
- **runtime** — VC++ redistributable / DLL load
- **Aviation** — plugin DSP/state bug

---

## Clean-machine gate (required before ship)

1. Fresh Windows user account or VM snapshot
2. Unzip package → follow `DOCS/INSTALL_WINDOWS.txt` only
3. Complete matrix above (at minimum: every FL version the client will use)
4. Remove VST3 folder → repeat install from README alone

**Priority rows.** If time is short, run these first — they are where the known bug lived:
*Sound recalled, not a sine*, *Render/export matches live*, and *Consolidate/bounce matches
live*. FLSI-008 reproduced on macOS with nothing more than load preset → play chords →
render, so it needs no save/reopen to show up. Windows has never been tested for it at all.

**The acceptance test, stated once:** choose a factory preset → save the project → close and
reopen FL → export/render/consolidate → the rendered audio must match what you heard live.
Repeat with a user preset, and with two instances on different presets.

**Automated cover.** `tests/HostStateRoundtripTests.cpp` already covers instance
independence across save/restore, independence through a shared release/prepare cycle, and
restore at a changed sample rate and block size — against the real `AviatorKeyzProcessor`.
Those run on every build, on every platform. What the host matrix adds that unit tests
cannot: FL's actual call ordering, its consolidate/bounce implementation, and per-version
wrapper behaviour. Do not treat a green unit suite as covering these rows.

**Retesting a replaced build:** quit FL completely first — it caches the loaded binary — and
confirm the commit SHA in the status bar / About overlay before trusting any result.
