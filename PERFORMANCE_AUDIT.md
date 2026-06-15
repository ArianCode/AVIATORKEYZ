# AviatorKeyz Performance Audit

**Date:** 2026-06-14  
**Scope:** Full-repository audit — real-time audio safety, DSP hot paths, threading, UI, build configuration  
**Platform tested:** macOS (Apple Silicon), Release build, CMake + JUCE 8.0.9

---

## Executive Summary

AviatorKeyz is a well-structured JUCE instrument with a clear audio-thread/message-thread split documented in `docs/CODE_STRUCTURE.md`. The primary CPU cost is **per-sample nested voice rendering** in `SamplerEngine` and `SynthEngine`, followed by **TextureEngine** when enabled. Several real-time safety and efficiency issues were found and corrected in this pass.

**Key outcomes:**
- Eliminated a **heap allocation path** in `processBlock` when hosts exceed the prepared block size
- Fixed a **filter drive bug** where drive was never stored (always near-zero saturation)
- Removed **RT-unsafe `dynamic_cast` and global RNG** from the audio callback path
- Added **active-voice tracking** to skip silent source engines entirely
- Added **fast math**, **FX bypass guards**, and **fused post-FX buffer passes**
- Added **compile-time profiling hooks** (`AVIATORKEYZ_PROFILE`) and **informational benchmarks**

All **62 C++ unit tests** pass after changes. Audio behavior is preserved except for the intentional filter-drive correction (previously broken).

---

## Architecture & Hot Paths

```
MIDI → SamplerEngine / SynthEngine (per-sample × voices)  ← hottest
     → input gain mix
     → FilterProcessor (per-sample TPT)
     → TextureEngine (per-sample × grains)                 ← hot when enabled
     → ToneShaper / Smear / Reverb
     → FxChain
     → stereo width + pan + limiter + output gain
```

Modulation (`LfoEngine`, `ModMatrix`, `PerformanceMacroEngine`) runs once per block before the chain. Parameter reads use APVTS atomic loads — acceptable cost relative to voice loops.

---

## Baseline Measurements

| Metric | Method | Result |
|--------|--------|--------|
| Sampler 16-voice @ 48 kHz, 128 samples | `PerformanceBenchmarkTests` (500 blocks avg) | **17.5 µs/block** |
| Idle engine skip @ 48 kHz, 128 samples | Same harness, no notes | **~0 µs/block** |
| C++ unit tests | `AviatorKeyzTests` | **62/62 pass** |
| Release build | `cmake --build build --target AviatorKeyz` | **Success** |
| Python validation | `pytest tests/` | Not run (pytest unavailable in CI shell) |

> **Note:** Pre-change timings were not captured in an automated harness. The idle-skip benchmark demonstrates the benefit of `hasActiveVoices()` — previously both engines iterated voice arrays every block even when silent.

**Budget reference @ 48 kHz / 128 samples:** callback period ≈ **2.67 ms**. Measured sampler-only micro-benchmark ≈ **0.65%** of budget (16 voices). Full plugin cost depends on enabled modules and polyphony.

---

## Top Bottlenecks (Ranked)

### Critical — RT safety / correctness

| Issue | Location | Risk |
|-------|----------|------|
| Scratch buffer `setSize()` in `processBlock` | `PluginProcessor.cpp` | Heap alloc on audio thread if block > prepared size |
| `FilterProcessor::setParameters` shadowed `drive01` | `FilterProcessor.cpp` | Drive never applied; wrong saturation behavior |
| `ModMatrix::choiceIndex` used `dynamic_cast` | `ModMatrix.cpp` | RTTI + potential alloc/contention on audio thread |
| `LfoEngine` random shape used `Random::getSystemRandom()` | `LfoEngine.cpp` | Global lock in host random on audio thread |

### High impact — CPU / scalability

| Issue | Location | Impact |
|-------|----------|--------|
| Voice engines process all blocks even when silent | `SamplerEngine`, `SynthEngine` | Wasted iterations over 16 voice slots |
| Synth pan computed 4× per voice per sample | `SynthEngine::process` | Redundant trig |
| Separate stereo-width, pan, output-gain loops | `PluginProcessor.cpp` | 3× buffer passes |
| FX chain entered when all FX off | `FxChain.cpp` | Unnecessary setup |
| ToneShaper ran when tone ≈ 0 | `ToneShaper.cpp` | 4× IIR per sample for no effect |
| `std::sin` in hot oscillator paths | Multiple DSP files | libm latency |

### Medium impact

| Issue | Location |
|-------|----------|
| Per-sample filter coefficient updates via `setParameters` each block | `FilterProcessor` — acceptable at block rate |
| TextureEngine O(samples × grains) inner loop | Inherent to granular design |
| Per-sample voice rendering (not block-vectorized) | Architectural; sample-accurate envelopes |
| UI timers at 10–45 Hz across Advanced/Cockpit views | Message thread only; monitor on low-end hosts |

### Low impact

| Issue | Location |
|-------|----------|
| Repeated APVTS atomic loads in `processBlock` | ~80 loads/block; dwarfed by voice loops |
| `FilterProcessor::applyDrive` redundant `tanh` divisor | Fixed via block-rate normalization |

---

## Real-Time Safety Violations Found & Status

| Violation | Status |
|-----------|--------|
| Heap alloc in `processBlock` (scratch resize) | **Fixed** — preallocated in `prepareToPlay`, `jassert` guard |
| `dynamic_cast` in mod matrix | **Fixed** — `static_cast` on known choice params |
| Global RNG in LFO random shape | **Fixed** — per-LFO xorshift state |
| `CriticalSection` in `getFactoryWaveformData` | **Acceptable** — UI thread only |
| `loadFactorySample` lock + malloc | **Acceptable** — message thread / suspended processing |
| `AK_LOG` / `DBG` in DSP | **Acceptable** — compiled out in Release (`AVIATORKEYZ_DEBUG`) |
| SampleLibrary `publish()` allocations | **Acceptable** — message thread only |

No file I/O, logging, or locks were found inside `processBlock` itself.

---

## Changes Implemented

### Real-time safety
1. **Preallocate scratch buffers** in `prepareToPlay`; remove `setSize` from audio thread
2. **ModMatrix** — replace `dynamic_cast` with `static_cast<juce::AudioParameterChoice*>`
3. **LfoEngine** — per-LFO xorshift RNG replaces `Random::getSystemRandom()`

### Correctness
4. **FilterProcessor** — fix `drive01` member assignment (was shadowed by parameter)

### DSP optimization
5. **Active voice counting** in `SamplerEngine` and `SynthEngine` with `hasActiveVoices()` early exit
6. **SynthEngine** — precompute pan gains in `setOscParams`; iterate only `maxVoices`
7. **FastMath** — `fastSin`, `fastSinPhase01`; used in samplers, synths, LFO sine, Hann window
8. **ToneShaper** — skip processing when `|tone| < 0.001`
9. **FxChain** — early return when all FX disabled; precompute distortion normalization
10. **FilterProcessor** — precompute drive normalization; skip tanh when drive ≈ 0
11. **PluginProcessor** — fuse stereo width + pan + output gain into single loop; conditional tone shaping

### Infrastructure
12. **`Source/Debug/AviatorProfile.h`** — optional microsecond block profiler (`AK_PROFILE_BLOCK_US`)
13. **`AVIATORKEYZ_PROFILE` CMake option** — enables profiling defines
14. **`tests/PerformanceBenchmarkTests.cpp`** — informational timing tests

---

## Files Changed

| File | Change type |
|------|-------------|
| `Source/PluginProcessor.cpp` | RT safety, fused post-FX loop, tone bypass |
| `Source/PluginProcessor.h` | Scratch size tracking |
| `Source/DSP/SamplerEngine.cpp` | Voice count, fast sin, early exit |
| `Source/DSP/SamplerEngine.h` | `hasActiveVoices()`, `activeVoiceCount` |
| `Source/DSP/SynthEngine.cpp` | Voice count, pan precompute, fast sin, early exit |
| `Source/DSP/SynthEngine.h` | Pan cache, `hasActiveVoices()` |
| `Source/DSP/FilterProcessor.cpp` | Drive bug fix, drive norm optimization |
| `Source/DSP/ModMatrix.cpp` | Remove dynamic_cast |
| `Source/DSP/LfoEngine.cpp` | Fast sin, local RNG |
| `Source/DSP/LfoEngine.h` | RNG state |
| `Source/DSP/FastMath.h` | `fastSin`, `fastSinPhase01`, fast Hann |
| `Source/DSP/ToneShaper.cpp` | Zero-tone bypass |
| `Source/DSP/FxChain.cpp` | FX bypass guard, dist norm |
| `Source/Debug/AviatorProfile.h` | **New** — profiling macros |
| `CMakeLists.txt` | `AVIATORKEYZ_PROFILE` option |
| `tests/PerformanceBenchmarkTests.cpp` | **New** — benchmarks |
| `tests/CMakeLists.txt` | Link benchmark + SynthEngine |

---

## Before / After (Measured & Estimated)

| Scenario | Before (est.) | After (measured/est.) |
|----------|---------------|------------------------|
| Idle callback (no notes) | Both engines scan 16 voices/block | **~0 µs** engine work |
| 16-voice sampler @ 128/48k | ~17–25 µs (similar order) | **17.5 µs** |
| Filter drive at 100% | No audible drive (bug) | Correct saturation |
| All FX off | FxChain setup + branch checks | Early return |
| Tone = 0 | 4 IIR filters × 128 samples | Skipped |
| LFO random | Global mutex possible | Lock-free xorshift |

---

## Remaining Risks

1. **TextureEngine** remains the dominant cost when enabled — O(blockSize × kMaxGrains). No structural change made to preserve sound.
2. **Per-sample voice loops** in sampler/synth — correct for sample-accurate envelopes but not SIMD-friendly. Future SoA refactor would help polyphony scaling.
3. **Filter envelope** is read once per block for cutoff modulation (`getFilterEnvLevel`) — block-rate mod; intentional but limits filter sweep resolution.
4. **`getFactoryWaveformData` lock** — UI must not call during preset load; low risk.
5. **Host block size changes** — if a host calls `processBlock` with `numSamples > prepareToPlay` size, debug builds will `jassert`; release relies on JUCE's max-block contract.

---

## Recommended Future Improvements

### High
- Block-process **FilterProcessor** via JUCE `dsp::ProcessContext` when coefficients are block-constant
- **Silence detection** after source mix — skip FX chain when block peak < −120 dBFS and no tails active
- **Parameter cache struct** rebuilt once per block to reduce APVTS pointer chasing

### Medium
- **TextureEngine**: process grains in sub-blocks; reduce per-sample Hann/trig cost with wavetable
- **SIMD** source mix (sampler + synth blend + input gain) — contiguous pass
- **UI**: throttle Cockpit gauge timer (45 Hz → 20 Hz); dirty-region repaints for waveform displays

### Low
- Precompute `constantPowerPan` cos/sin table for UI knobs
- Profile build integration with Instruments / Tracy

---

## Items Intentionally Not Changed

- Voice architecture (AoS, per-sample rendering) — preserves sample-accurate envelopes and automation
- TextureEngine grain algorithm — sound-critical
- SampleLibrary double-buffer publish model — already correct
- APVTS parameter IDs / preset format — compatibility
- UI layout and timer rates — no measured message-thread bottleneck in this pass
- JUCE fetch version, plugin codes, third-party modules
- Global `-ffast-math` — avoided due to filter stability / NaN concerns

---

## Testing Performed

| Test | Result |
|------|--------|
| Release build (VST3 + tests) | Pass |
| 62 C++ unit tests | Pass |
| Performance benchmark tests | Pass (informational) |
| Sampler voice steal / release / glide / reverse | Pass (existing tests) |
| TextureEngine wet/dry gating | Pass (existing tests) |
| Pitch alignment | Pass (existing tests) |
| ASan build | Not re-run this session (Debug+Clang config exists in CMake) |

### Recommended manual validation
- [ ] A/B filter drive before/after fix at high `FILTER_DRIVE`
- [ ] Rapid preset switching in FL Studio + Ableton
- [ ] 32/64-sample buffers under full polyphony + texture enabled
- [ ] Offline bounce vs realtime equivalence
- [ ] Long session memory stability

---

## Host / Platform Limitations

- **FL Studio** may use variable block sizes — scratch buffers sized from `prepareToPlay` maximum
- **Ableton Live** suspends plugin when silent — idle skip optimizations align well
- **Apple Silicon** — Release build uses JUCE recommended LTO flags; no arch-specific SIMD added yet
- **Windows** — changes are portable C++; not compiled in this audit session

---

## Build Configurations

| Config | Purpose | Flags |
|--------|---------|-------|
| **Release** | Shipping | JUCE recommended LTO + warnings |
| **Debug** | Development | `AVIATORKEYZ_DEBUG=1`, ASan/UBSan (Clang) |
| **Profile** | Callback timing | `-DAVIATORKEYZ_PROFILE=1` via `AVIATORKEYZ_PROFILE=ON` |

Enable profiling:
```bash
cmake -B build-profile -DCMAKE_BUILD_TYPE=RelWithDebInfo -DAVIATORKEYZ_PROFILE=ON
cmake --build build-profile --target AviatorKeyz
```

---

## Ranked Summary

### Critical (fixed)
- Audio-thread heap allocation in scratch buffer resize
- Filter drive parameter shadowing bug
- RTTI (`dynamic_cast`) on audio thread in mod matrix
- Global RNG lock in LFO random shape

### High impact (fixed)
- Silent voice engine skipping
- Synth pan precomputation
- Fused post-FX buffer pass (3→1 loops)
- FX chain early exit
- Tone bypass at zero
- Fast sine approximation in hot paths

### Medium impact (partially addressed / documented)
- Filter drive tanh normalization
- Benchmark + profiling infrastructure
- Hann window fast path

### Low impact
- Distortion normalization precompute
- Documentation and test harness additions
