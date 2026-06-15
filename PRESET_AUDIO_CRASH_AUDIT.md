# Preset Audio & Crash Audit

Date: 2026-06-14  
Project: AviatorKeyz VST3  
Build tested: `build-asan` Debug + AddressSanitizer/UndefinedBehaviorSanitizer (Clang)

## Reproduction sequence

1. Load plugin (Standalone or VST3 host).
2. Open preset browser / cockpit preset bar.
3. Select any factory preset (e.g. **Leads → Init** or **BOS_AA_Synth_One_Shot_Shadows_C**).
4. Send MIDI note-on (velocity > 0), e.g. C4 (60).
5. Observe: before fix — silence and/or crash shortly after preset switch.
6. Repeat rapid preset next/previous while holding notes.
7. Repeat with editor closed, 44.1/48 kHz, buffer 128/512/1024.

### Scope of failure (before fix)

| Condition | Result |
|-----------|--------|
| Every factory preset tested in regression harness | Silent or crash-prone |
| After switching presets | Yes — primary trigger |
| Sample-based presets | Yes |
| Synth blend > 0 | Secondary path only if `source_blend` not forced to 0 |
| Async loading | No separate async path; synchronous publish on message thread |
| Editor open/closed | Both |
| Release vs Debug ASAN | Crash more visible under concurrent preset load in Debug/ASAN |
| Host-specific | Any host where `setStateInformation` or preset UI runs without audio suspend |

## Version control

| Item | SHA / note |
|------|------------|
| Last known good baseline | `a5ac31e` — factory per-preset bank migration; committed processor still called `samplerEngine.process()` unconditionally |
| First failing changes | **Uncommitted WIP** on top of `a5ac31e` (not a separate commit): performance edits in `PluginProcessor.cpp`, `SamplerEngine.cpp` |
| Bisect note | Full git bisect not run; diff isolated regressions to uncommitted processor/engine changes |

## Root causes

### 1. Silent audio — `hasActiveVoices()` gate in `processBlock`

**File:** `Source/PluginProcessor.cpp`

WIP change:

```cpp
if (sourceBlend < 0.999f && samplerEngine.hasActiveVoices())
    samplerEngine.process (samplerScratch);
```

`noteOn()` runs earlier in the same callback and increments `activeVoiceCount`, but the new counter could desync from `Voice::active` during legato/mono steals and preset `allSoundOff()` races. More importantly, gating duplicated the early-return inside `SamplerEngine::process()` without adding safety — if the counter was wrong, rendering was skipped entirely while MIDI still fired.

**Fix:** Removed the gate; always call `samplerEngine.process()` / `synthEngine.process()` when the source blend allows (matching committed `a5ac31e` behaviour). Engines already return immediately when idle.

### 2. Crash — preset sample reload without guaranteed audio suspend

**File:** `Source/PluginProcessor.cpp` — `loadFactorySample()`

`loadFactorySample()` mutates:

- `SampleLibrary` mono storage (double-buffer publish frees the inactive slot)
- `SamplerEngine` voices (`allSoundOff()`, new snapshot pointer)

Previously `suspendProcessing(true)` ran **only on the message thread**. Host calls to `setStateInformation()` and some preset paths could reload samples **while the audio thread was still rendering**, causing use-after-free reads of `Voice::sampleData`.

**Fix:** Always wrap sample reload in `suspendProcessing(true/false)` regardless of calling thread.

### 3. Silent / unstable parameters — partial preset `replaceState`

**File:** `Source/State/ApvtsStateHelpers.cpp`

Factory presets store ~12 `<PARAM>` nodes. Old flow:

1. `resetApvtsToDefaults()`
2. `apvts.replaceState(partialTree)` — replaced entire APVTS tree with sparse children
3. Patch a few advanced defaults

Replacing the full tree with a partial factory tree could leave APVTS in an inconsistent state (missing property nodes, wrong normalized values) and risk `source_blend` staying > 0 so MIDI skipped the sampler.

**Fix:** For partial factory presets, merge `<PARAM>` children onto layout defaults via `setValueNotifyingHost()` and **do not** call `replaceState()`. Full host/user states still use `replaceState()`.

### 4. Secondary risk — preallocated scratch buffers

WIP change preallocated scratch buffers and skipped per-block `setSize()`. If a host ever delivered `numSamples > prepareToPlay` block size, out-of-bounds writes were possible (Release: crash; Debug: `jassert`).

**Fix:** Restored per-block `setSize(2, n, …)` for sampler/synth scratch buffers.

## Audio path verification (harness)

Signal traced in `tests/PresetPlaybackTests.cpp`:

`PresetManager.loadPreset` → `onPresetLoaded` → `SampleLibrary.publish` → `SamplerEngine.setSampleSnapshot` → `MidiHandler.process(noteOn)` → `SamplerEngine.process` → peak > `1e-4`.

| Stage | Init preset | BOS_AA preset | Leads/Bells/Arps spot-check |
|-------|-------------|---------------|-----------------------------|
| Preset parse | OK | OK | OK |
| Embedded WAV resolved | OK | OK | OK |
| Published snapshot regions | ≥1 | ≥1 | ≥1 |
| `source_blend` after load | 0.0 | 0.0 | 0.0 |
| MIDI → sampler noteOn | OK | OK | OK |
| Peak @ MIDI 60 | > 1e-4 | > 1e-4 | > 1e-5 |
| Rapid switch × 8 rounds | no crash | no crash | finite peaks |

No temporary debug instrumentation remains in the audio callback.

## Files changed

| File | Change |
|------|--------|
| `Source/PluginProcessor.cpp` | Always suspend around sample reload; restore scratch `setSize`; remove `hasActiveVoices()` gate |
| `Source/PluginProcessor.h` | Remove unused `scratchBlockSize` |
| `Source/State/ApvtsStateHelpers.cpp` | Merge partial factory presets without `replaceState` |
| `tests/PresetPlaybackTests.cpp` | **New** regression tests |
| `tests/CMakeLists.txt` | Link new test + `MidiHandler.cpp` |

## Ownership & threading

| Resource | Owner | Handoff |
|----------|-------|---------|
| Sample mono buffers | `SampleLibrary` double buffer | Published snapshot pointer; reload only while audio suspended |
| Active voices | `SamplerEngine` | Cleared in `loadFactorySample()` before snapshot swap |
| APVTS parameters | `AudioProcessorValueTreeState` | Partial presets merged in-place; no tree replacement |
| Preset metadata | `PresetManager` | Message thread; callback triggers suspended sample reload |

No locks added to `processBlock`.

## Tests added

- `PresetPlaybackTests` (5 cases): Init load+audio, BOS_AA audio, `source_blend==0`, rapid switching, multi-category spot-check.
- Existing suites still pass: **67 tests, 0 failures** (`build-asan/AviatorKeyzTests`).

Run:

```bash
cmake --build build-asan --target AviatorKeyzTests
./build-asan/tests/AviatorKeyzTests_artefacts/Debug/AviatorKeyzTests --category AviatorKeyz
```

## Preset compatibility

- Factory XML format unchanged.
- Partial preset semantics preserved: unstored params reset to layout defaults; `source_blend` forced to 0 (sample-only) for factory presets.
- Host full-state recall still uses `replaceState()`.

## Remaining risks

1. **Held-note crossfade:** Reload still hard-cuts via `allSoundOff()` (intentional for sample swap; deferred-release pool not implemented).
2. **Very large block sizes:** Per-block scratch `setSize` allocates on the audio thread when block size changes — acceptable for correctness; revisit only after stability sign-off.
3. **Full preset corpus:** Regression tests spot-check categories; run `python3 -m pytest tests/test_presets.py` for XML/schema coverage.

## Shared root cause?

**Partially.** Both symptoms stemmed from unsafe preset switching, but via different mechanisms:

- **Silence:** `hasActiveVoices()` skip + partial `replaceState` parameter/routing issues.
- **Crash:** Sample buffer use-after-free when reload ran concurrently with audio.

Fixing suspend + merge + process routing addresses both in practice.
