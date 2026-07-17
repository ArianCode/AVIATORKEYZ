# AviatorKeyz — Full Codebase, DSP, UX & Release-Readiness Audit

**Date:** 2026-07-16
**Scope:** Entire repository at `main` (72c02e3), ~18k lines of plugin source, all DSP/State/GUI/MIDI layers, build system, tests, docs.
**Method:** Every DSP and state file read in full; signal paths traced from parameter → DSP → output; math claims verified numerically; unit tests built and run (112/112 pass).
**Rule followed:** No code was modified.

---

## 1. Executive summary

AviatorKeyz has a **genuinely solid core**: the sample-playback path (SampleLibrary double-buffer → SamplerEngine voice pool → category playback policies) is well designed, tested (112 passing unit tests), and thread-safe in its main handoff. Preset/state discipline (frozen parameter IDs, schema versioning, migration helpers) is above average for a project at this stage.

Around that core, however, is a **large shell of disconnected and broken features**. Roughly **150 of the 249 registered parameters do nothing audible**: the entire synth engine, filter section, 3 LFOs, and the 8-row mod matrix are defined, saved, automatable, and partially visible in the UI — but never processed. The performance-FX layer (stutter/half-time/freeze/tape-stop/pitch-drop) is between crude and outright non-functional. Three confirmed math bugs make shipped-sounding features audibly wrong: the **constant-power pan law is broken** (the plugin's stereo image is left-skewed at all times), the granular **"Hann" window is a sine, not a cosine** (clicky grains), and the **chop sequencer's clock confuses samples with seconds** (the 16-step engine is nonfunctional).

Most urgent of all: a **debug logger that writes to a hardcoded path on the developer's machine is compiled into release builds and called from the audio thread** (`processBlock`, state save/restore, preset load — including a stack-backtrace capture). This is a release blocker on real-time-safety, performance, and privacy grounds simultaneously.

The right strategy is **not** a rewrite. It is: fix the 6 confirmed blockers, then **hide everything that isn't wired** (synth, filter, LFO, mod matrix, broken perf FX), ship the sampler that already works, and reintroduce the advanced systems one at a time when their DSP actually exists.

## 2. Overall health score: 4.5 / 10

| Axis | Score | Note |
|---|---|---|
| Core sampler DSP | 7.5/10 | Solid, tested; env/retrigger click edge cases |
| RT safety | 3/10 | File I/O + allocations on audio thread (confirmed) |
| Feature completeness vs UI promise | 2.5/10 | ~60% of parameter surface is dead |
| State/preset management | 7/10 | Strong schema discipline; minor gaps |
| UX coherence | 3.5/10 | Aviation metaphor obscures function; dead & duplicated controls |
| Code quality | 6/10 | Clear layering, but dead code + stale docs |
| Test coverage | 6.5/10 | Good for voices/state; zero coverage of pan/window/chop math |

---

## 3. Architecture summary

```
processBlock (PluginProcessor.cpp:271)
 ├─ read EngineState from APVTS      (PerformanceApvtsReader::readBaseState — ~330 param reads/block)
 ├─ apply 4 perf macros              (MacroMapper::applyMacros)
 ├─ PhraseChopper::updatePlayback    → ChopPlaybackState → SamplerEngine
 ├─ MidiHandler::process             → SamplerEngine.noteOn/Off  (SynthEngine branch: sourceBlend hardcoded 0 → dead)
 ├─ SamplerEngine::process           (16-voice mono sum → both channels)
 ├─ input gain (smoothed)
 ├─ PerformanceTexturePipeline       (PhraseChopper gate → MotionEngine → TextureBlendEngine[granular])
 ├─ PerformanceFxEngine              (tape stop / filter sweep; pitch drop is a no-op)
 ├─ brightnessShaper (STEREO_WIDTH id!) → toneShaper (TONE) → smearProcessor (HPF macro, SMEAR id)
 ├─ reverbTail → fxChain (delay/chorus/lofi/dist)
 └─ limiter → pan (broken law) → output gain (smoothed)
```

Declared in the processor but **never in the chain**: `SynthEngine`, `FilterProcessor`, `TextureEngine` (a second, orphaned instance), and — never even instantiated — `LfoEngine`, `ModMatrix`, `PerformanceMacroEngine`.

State layer: APVTS + `StateSchema.h` (frozen IDs) + `PresetManager` (embedded factory XML/WAV via BinaryData, user presets in `~/Documents/AviatorKeyz/Presets`). GUI: cockpit-photo main page (`CockpitCrossworldPanel`) + a small Advanced page (`AdvancedPageContent` — performance params only). A large set of Advanced components (`LfoPanelComponent`, `ModMatrixComponent`, `SynthPanelComponent`, `PhrasePanelComponent`, `SourcePanelComponent`, `FilterCurveGraph`, `FxAdvancedPanel`, `TextureVisualizerComponent`) and all of `GUI/_legacy/` are **dead code** — compiled, never instantiated.

---

## 4. What is implemented well (protect these)

1. **SampleLibrary double-buffer publish** ([SampleLibrary.cpp:313](Source/State/SampleLibrary.cpp)) — atomic read-index swap, mono conversion off the audio thread, WAV `smpl`-chunk root-note parsing with sensible precedence (chunk > preset XML > name inference).
2. **Voice pool & policies** — fixed 16-voice pool, steal-release-quietest-then-quietest, per-note FIFO stacks for correct same-note note-off pairing, 5 ms choke fades, `NoteGatePolicy`/`RetriggerPolicy` derived from category+sound type (`CategorySoundPolicy.h`). This is the musical heart and it is mostly right.
3. **State schema discipline** — `StateSchema.h` frozen-ID contract, `STATE_SCHEMA_VERSION`, `migrateLegacyAdvancedParams()`, legacy stereo-width→brightness value migration in `ApvtsStateHelpers.cpp:43`.
4. **Preset identity in host state** — `getStateInformation` stores category/name/sampleId/rootNote; projects reopen with the right sample.
5. **`suspendProcessing` around sample swap** + deferred preset load via `Timer::callAfterDelay` (MainPanel.cpp:23) to escape the mouse-event stack — the right instinct.
6. **Test suite** — 112 tests covering voice lifecycle, retrigger/choke, pitch alignment, sustain pedal, state schema, preset playback. Build infra (sanitizer separation after FLSI-006, `known-host-issues.md`) is disciplined.
7. **Bus layout** correctly restricted to stereo-out instrument; `ScopedNoDenormals`; FX bypass guards avoid processing disabled FX.

---

## 5. Critical problems (all confirmed by code inspection; math verified numerically)

### C1 — Debug logger does file I/O on the audio thread and ships in release
`Source/Debug/AgentDebugLog.h` opens an `std::ofstream` appending to a **hardcoded absolute path** (`/Users/ariangholamipour/.../.cursor/debug-0af0ef.log`). It is **not gated by any build flag** (only `AK_LOG` is gated by `AVIATORKEYZ_DEBUG`; `AgentDebugLog` is not). Call sites include:
- `processBlock` (PluginProcessor.cpp:283–307): every block increments atomics, calls `setSize`, and on block 1 and every 2000th block builds JSON strings and opens the file — **on the audio thread**.
- `PresetManager::loadPreset` (PresetManager.cpp:223): captures `SystemStats::getStackBacktrace()` (expensive, allocating) on **every preset load**.
- `getStateInformation`/`setStateInformation`, editor ctor/dtor, `ApvtsStateHelpers`, `getPresetsForCategory`.
**Action:** delete the header and all `#region agent log` blocks (or gate behind `AVIATORKEYZ_DEBUG` *and* a non-hardcoded path). **Blocker. Confidence: confirmed.**

### C2 — Constant-power pan law is mathematically wrong; output is always left-biased
`AviatorFastMath::constantPowerPan` (FastMath.h:23) uses `ang = (pan+1) · π/8`; correct is `(pan+1) · π/4`. Verified: center pan yields **L=0.924, R=0.383** (≈8 dB imbalance); hard right yields L=R=0.707 (i.e., "hard right" = true center). After the `√2` makeup in `processBlock` (PluginProcessor.cpp:463-465), every preset at default settings plays **~+2.3 dB left / −5.3 dB right**. Also affects `SynthEngine` osc pan (dead today) and `TextureEngine` grain pan.
**Action:** change `halfPi * 0.25f` → `halfPi * 0.5f`. Note: this **changes the sound of all existing sessions** (they become correctly centered) — do it before release, not after. **Blocker. Confidence: confirmed (numerically).**

### C3 — Real-time allocations in `processBlock`
- `ParamID::chopStepParamId()` (StateSchema.h:317) builds `String::formatted` — `PerformanceApvtsReader::readBaseState` calls it **80× per audio block** (16 steps × 5 params) → 80 heap allocations + 80 string-hash map lookups per block, every block (PerformanceApvtsReader.cpp:47-55).
- `ToneShaper::updateCoeffs` (ToneShaper.cpp:42-46) calls `IIR::Coefficients::makeLowShelf/makeHighShelf` — **heap-allocating** `ReferenceCountedObject`s — whenever tone/brightness moves >0.002 (i.e., continuously during automation), ×2 instances (tone + brightness).
- `samplerScratch.setSize(2, n, …, true)` every block (PluginProcessor.cpp:305) and `TextureBlendEngine::dryCopy.setSize` (TextureBlendEngine.cpp:29): no-ops until a host delivers a block larger than prepared — then they **allocate on the audio thread**. FL Studio is known to deliver variable block sizes.
**Action:** cache raw `std::atomic<float>*` for all chop-step (and other) params once at construction; precompute shelf coefficients into preallocated arrays or use a filter whose coefficients can be set in place; clamp `n` to prepared size (or `jmin`) instead of `setSize`. **Blocker. Confidence: confirmed.**

### C4 — The chop/step-sequencer clock is broken (samples vs seconds)
`PhraseChopper::stepIndexForClock` (PhraseChopper.cpp:27) divides `sampleCounter` (incremented **1 per sample**, i.e., counts samples) by `stepLen` (**seconds**). Verified at 120 BPM / 1/16: successive samples map to steps 0, 8, 0, 8… The step pattern, swing, per-step volume/offset/reverse/pitch (80 parameters!) are all effectively random noise gates. Additionally `updatePlayback` fires once per block (slice boundaries quantized to block size) and `stepGainSmoothed` jumps rather than smooths (PhraseChopper.cpp:129).
**Action:** keep the counter in samples and compare against `stepLen * sampleRate`, or accumulate seconds by `n/sampleRate`; process step transitions at sample offsets within the block. **Blocker if Chop ships; otherwise hide Chop. Confidence: confirmed (numerically).**

### C5 — Granular window is not a Hann window → clicky texture layer
`AviatorFastMath::hannWindow` (FastMath.h:51) computes `0.5·(1−sin(2πp))`: verified it **starts and ends at 0.5** (discontinuity = click at every grain boundary) and peaks at p=0.75. Correct is `0.5·(1−cos(2πp))`.
**Action:** one-character-class fix (`fastSin` of shifted phase or a cos variant). Changes texture sound (for the better). **Blocker if Texture ships. Confidence: confirmed (numerically).**

### C6 — Texture mix double-counts the dry signal
`TextureEngine::process` already outputs `dry·(1−mix) + wet·mix` internally (TextureEngine.cpp:256), then `TextureBlendEngine::process` **adds the dry copy again** scaled by `(1−mix)` (TextureBlendEngine.cpp:52-56). At mix=0.5 the dry path is 2×(1−mix)=1.0 plus wet — the MIX knob acts as a loudness boost, not a blend.
**Action:** pass wet-only mode to TextureEngine or remove the external dry add. **High. Confidence: confirmed.**

---

## 6. Feature-status matrix

| Feature | Status | Files | Root cause / note | Action | Prio | Cx |
|---|---|---|---|---|---|---|
| Sample playback (chromatic) | **Working** | SamplerEngine, SampleLibrary | Tested | Keep | — | — |
| Phrase/one-shot playback policies | Working, needs improvement | CategorySoundPolicy.h | Gate/retrigger policies good; one-shot `TriggerToEnd` ignores note-off (bleed, see §11) | Add voice cap / choke option | High | M |
| Preset system (factory+user) | Working, needs improvement | PresetManager, FactoryResources | Double WAV decode per load; save silently overwrites; favorites not persisted | Fix UX gaps | High | L–M |
| Host state save/restore | Working | PluginProcessor 511–558 | Identity + params restored | Keep | — | — |
| Glide/portamento | Working | GlideEngine | Per-sample rate recompute is wasteful but correct | Optimize later | Low | L |
| Reverse | Working | ReversePlayer, SamplerEngine | Duplicated params: `reverse` OR `src_reverse` (OR'd) | Consolidate | Med | L |
| Envelope (ADSR) | Working, has click bugs | SamplerEngine 395–408, 517 | Attack=0 default + uninit decay segment → 1→sustain jump; mono retrigger uses `allSoundOff` (no fade) | Fix | **Blocker-adjacent** | L |
| Tone / Filter(HPF) / Reverb / Delay / Chorus / LoFi / Dist | Working | ToneShaper, SmearProcessor, ReverbTail, FxChain | RT alloc in ToneShaper (C3); delay is fine | Fix C3 | High | L |
| Output limiter | Working | OutputLimiter | JUCE limiter, on by default | Keep | — | — |
| Pan | **Broken** | FastMath.h:23 | C2 pan law | Fix | **Blocker** | L |
| Chop step sequencer (16 steps, 80 params) | **Broken** | PhraseChopper | C4 clock bug; steps have no UI grid anyway | Fix or defer+hide | **Blocker/defer** | M |
| Texture layer (PTEX_*) | **Broken-ish** | TextureBlendEngine, TextureEngine, FastMath | C5 window + C6 double-dry; engine core is decent | Fix both | High | L–M |
| Performance macros (4) | Working but opaque | MacroMapper, MacroPresetParser | Category defaults; positive-amount mappings offset targets even at rest if default ≠ mapped zero (defaults are 0.5 = bipolar center — verify per mapping) | Audit mappings; show macro targets in UI | Med | M |
| Perf FX: Stutter | Poor | MotionEngine 44–69 | Buffer-size dependent, mono | Redesign or remove | Med | M |
| Perf FX: Reverse | Poor | MotionEngine 71–78 | Reverses each block → garble, block-size dependent | Remove | Med | L |
| Perf FX: Half-time | **Broken** | MotionEngine 95–102 | In-place `L[i]=L[i/2]` reads already-overwritten samples → degenerates to held first sample | Remove or rewrite with ring buffer | High | M |
| Perf FX: Freeze | **Broken (no-op)** | MotionEngine 117–134 | read==write index → passthrough | Rewrite or remove | High | M |
| Perf FX: Scatter | Poor | MotionEngine 104–115 | Random in-block sample swaps; depends on `chop.random` from another feature | Remove | Low | L |
| Perf FX: Tape stop | Misleading | PerformanceFxEngine | Volume fade only, no pitch; retriggers forever while latched | Rework as momentary + pitch | Med | M |
| Perf FX: Pitch drop | **No-op** | PerformanceFxEngine.cpp:94 | `pitchDropSemitones` explicitly `ignoreUnused` | Remove param from UI or implement | High | M |
| Perf FX: Filter sweep | Broken-ish | PerformanceFxEngine 77–92 | Filter state reset every block (clicks); no UI | Remove | Low | L |
| Synth engine (osc1/2, 13 params + source_blend) | **Disconnected** | SynthEngine (fully written!), MidiHandler call w/ `0.f` (PluginProcessor.cpp:354) | Never prepared, never processed | Defer + hide params from UI | High | M to wire |
| Filter section (filter_*, env_flt_*, 10 params) | **Disconnected** | FilterProcessor (exists, unused) | Never processed | Defer + hide | High | M |
| LFO 1–3 (15 params) | **Disconnected** | LfoEngine (exists, never instantiated) | No DSP hookup | Defer + hide | High | M–H |
| Mod matrix (32 params) | **Disconnected — worse: UI shows fake mod state** | ModMatrix (never instantiated), ModRoutingHub, PrecisionKnob.cpp:183 | Knobs render modulation rings for routings that never run | Hide UI affordance | High | M |
| Old texture engine (TEX_*, 15 params) | Superseded/dead | processor member `textureEngine` unused | Replaced by PTEX_* pipeline | Remove member; keep IDs for compat | Med | L |
| Phrase params (PHRASE_*, 8) | Superseded | migrated to SRC_* by `migrateLegacyAdvancedParams` | `PHRASE_TRIGGER_MODE`, `PHRASE_KEY_SYNC` never read | Hide; keep IDs | Med | L |
| "Time Stretch" playback mode | **Fake** | PerformanceTypes.h:31, SamplerEngine.cpp:476 | Behaves identically to PhraseOriginal; no stretch engine | Remove from choice list (UI) until real | High | L |
| BPM sync / speed | **Wrong model** | SamplerEngine.cpp:482–496 | Sync repitches (resample), assumes 4-beat phrase, ignores ORIG BPM in window branch | See §11 | High | M–H |
| Cockpit dead buttons (TAXI/CRUISE/CLIMB/DESC, V1/panic, EMER) | **Dead UI** | CockpitZones.cpp, CockpitCrossworldPanel.cpp:190–197 | Actions `flightMode`/`panic`/`emergencyBurst` never handled | Remove or wire | High | L |
| Toggles bound to continuous params | **Broken UI** | CockpitZones.cpp:8–9 | "CAB" toggle slams TONE to ±1; "HYD" slams reverb_amount 0/1 | Rebind or remove | High | L |
| Favorites | GUI-only state | CockpitCrossworldPanel (std::set) | Lost on close; not in saved state | Persist to user file | Med | L |
| Legacy GUI tree (`GUI/_legacy/*`, 8 Advanced panels) | Dead code | — | Compiled, never instantiated | Delete from build | Med | L |
| `processBlockBypassed` param reads | Dead code | PluginProcessor.cpp:486–496 | reads then `ignoreUnused` | Clean | Low | L |
| `releaseResources` double `fxChain.reset()` | Dead code | PluginProcessor.cpp:259–260 | typo | Clean | Low | L |

**Count check:** 249 registered parameters (`kExpectedApvtsParamCount`); ~99 are connected to sound (core + FX + SRC_* + chop master + PTEX_* + macros), of which chop-step (80) and several perf-FX ones are connected to broken DSP. ~105 (LFO 15, mod 32, osc 13, source_blend 1, filter 10, TEX 15, PHRASE 8, perf-fx dead 3+) are pure state ballast.

---

## 7. Disabled / disconnected report — how each chain breaks

Chain notation: UI → param → DSP → sound.

- **Synth**: no UI (SynthPanelComponent dead) → params exist → `MidiHandler::process(..., sourceBlend=0.f)` hardcoded at PluginProcessor.cpp:354 → `useSynth` always false; `synthEngine.process` never called; not prepared in `prepareToPlay`. Chain broken at **processor**.
- **Filter**: no UI → params exist → `filterProcessor` member never referenced outside declaration. Broken at **processor**.
- **LFO/ModMatrix**: partial UI leftovers (PrecisionKnob mod rings via `ModRoutingHub::stateForParam`) → params exist → **no DSP instance anywhere**. Broken at processor; UI actively misleads (shows routing state that has no effect).
- **Chop steps**: no step-grid UI in the live Advanced page → 80 params → PhraseChopper reads them but its clock is broken (C4). Broken at **DSP math**.
- **Pitch drop / scatter / tape-stop / filter-sweep**: PERF_FX_TAPE_STOP, SCATTER, PITCH_DROP, FILTER_SWEEP have **no UI cells** (AdvancedPageContent shows only stutter/rev/half/freeze); pitch-drop DSP is a stated no-op. Broken at **both ends**.
- **PHRASE_KEY_SYNC / PHRASE_TRIGGER_MODE**: params defined, migrated, never read by any engine.
- **Cockpit actions**: `flightMode`, `panic`, `emergencyBurst`, `reverbBypass` strings in CockpitZones.cpp are never matched in `buildPhotoAnchors` (only presetPrev/presetNext/library) → visible, clickable, dead.

---

## 8. Broken-feature report (works differently than intended)

Beyond C2, C4, C5, C6 above:

1. **Zero-attack click + uninitialized decay** — `startVoice` with `attackMs<=0` (the default: ENV_ATTACK default is 0, not the 5 ms the schema comment claims) enters `EnvStage::decay` with `envSegSamplesLeft`/`envLinearStep` stale from reset (SamplerEngine.cpp:395-400) → one sample later env snaps from 1.0 to `sustainLevel`. Also every note starts mid-waveform at full gain → clicks on non-zero-crossing samples. Fix: minimum 1–3 ms attack ramp; initialize decay segment.
2. **Mono/legato retrigger clicks** — `noteOn` in mono mode calls `allSoundOff()` (hard reset, no fade; SamplerEngine.cpp:645) and `startVoice` on an active voice "chokes" then immediately overwrites the envelope (SamplerEngine.cpp:326-327) — the choke fade never renders. Fix: steal via a short fade-out on a *different* voice slot, or render the choke.
3. **Same-note stacking in poly Gated mode is choke-based** — `chokeSameNoteVoices` on every retrigger (5 ms fade) is fine, but `NoteGatePolicy::TriggerToEnd` ignores `noteOff` *and* `allNotesOff` (SamplerEngine.cpp:686-688) — a DAW "stop" that sends All-Notes-Off will **not** stop one-shots; only All-Sound-Off does. In Ableton/FL, stopping the transport can leave tails ringing to sample end. Fix: honor allNotesOff always, or at minimum on transport stop.
4. **`stepGainSmoothed`/chop gate applied once per block** — `chopState.gateGain` multiplies the *whole block* in `renderVoiceSample` via `chokeGain` (constant per block) and again in `PhraseChopper::process` → double gating of the same signal (once inside sampler, once after) — level dips are squared.
5. **Sine fallback masking errors** — missing sample region silently plays a sine (SamplerEngine.cpp:379-390, 614-621). Users hear a mystery beep instead of an error state. Fix: silence + UI error badge.
6. **Preset save** — `saveCurrentPreset` (CockpitCrossworldPanel.cpp:98) writes `<currentName>.xml` into the user dir with no dialog, no rename, no overwrite warning, no success feedback.
7. **`restoreFxParams` uses `setValue`** not `setValueNotifyingHost` (PresetManager.cpp:77) — host automation lanes go stale after a preset load with FX-lock on.

---

## 9. Overcomplicated-feature report

1. **Two overlapping performance systems**: `PERF_MODE` (8-value enum: Normal/Chop/Gate/Stutter/HalfTime/Reverse/Scatter/Freeze) *and* 8 independent `PERF_FX_*` bool latches, OR'd together inside MotionEngine. Same effects, two control surfaces, latching semantics for momentary gestures. → Keep **one** system of momentary performance buttons; delete the enum or the bools.
2. **Two texture engines** (TEX_* + PTEX_*), two phrase systems (PHRASE_* + SRC_*), two reverse params, two gain params (INPUT_GAIN labeled "ENGINE" on an instrument with no input, plus OUTPUT_GAIN). → One of each, aliases kept only for state compat.
3. **Macro system**: 4 macros × 8 mappings with curves, parsed from preset XML plus per-category defaults — powerful, but invisible: the user sees 4 knobs named "Chop/Texture/Space/Tone" with no indication of what they touch. → Show mapping targets in the macro strip tooltip.
4. **The cockpit metaphor** triples control count: glide appears as THROTTLE (gauge), GLD (knob), PITCH (lever), LFO (fader); brightness as WINGS/BRT/VEL; tone as CAB toggle/TONE knob/FLT lever. Producers can't find "attack" among TURBULENCE and HYD. → One control per parameter, plain names (aviation styling can stay visual, not lexical).
5. **`SamplerEngine` playback-rate logic** — `updateVoicePlaybackRates` + `voiceReadIncrement` recompute mode logic per sample during glide; the `PlaybackRates` separation (source/pitch/time) is a good design ("never collapse into one ratio") that the DSP then… collapses into one ratio because there's no stretch engine. Honest simplification: two modes for v1 — *Keys* (ChromaticResample) and *Phrase* (original speed) — until a real stretcher exists.

---

## 10. Real-time audio-safety report

**Confirmed violations** (in the callback path):
| # | Where | What |
|---|---|---|
| RT1 | PluginProcessor.cpp:283–307 | `AgentDebugLog::write` → `std::ofstream` open/append + String building (block 1, every 2000th block) |
| RT2 | PerformanceApvtsReader.cpp:47–55 | 80 × `String::formatted` heap allocations + string map lookups per block |
| RT3 | ToneShaper.cpp:42–46 | `IIR::Coefficients::make*` heap allocation on tone/brightness change (automation = every block), ×2 instances |
| RT4 | PluginProcessor.cpp:305, TextureBlendEngine.cpp:29 | `AudioBuffer::setSize` on audio thread — allocates when host block > prepared block |
| RT5 | PluginProcessor.cpp:311–317 | `getPlayHead()->getPosition()` fine; but `EngineState` copies (2×/block) carry `juce::String` members (`MacroControl::name`) → atomic refcount churn; benign but unnecessary |
| RT6 | MidiHandler.cpp:67–80 | `DBG` + String building per note-on (debug builds only — gated by JUCE_DEBUG) |

**Potential risks:**
- `suspendProcessing(true)` from `setStateInformation` — correct, but any host calling setState on a non-message thread while the GUI also loads a preset relies on `sampleLoadLock` ordering; OK today, fragile.
- `macroControls` (array of structs with `juce::String`) written on message thread in `onMacroMapsLoaded` while read in `processBlock` — **data race** (non-atomic, no lock). Low probability, real. Swap via pointer + atomic, or copy under `suspendProcessing`.
- `Random::setSeedRandomly()` in `prepare/reset` — message thread, fine; `rng.nextFloat()` in audio path is JUCE Random (fine, no locks).
- Choke/steal behavior under voice pressure: `findFreeOrStealVoice` steals the quietest and restarts it **instantly** (no fade) → audible steal clicks under 16-voice pressure.

**Sample-replacement safety:** good (suspend + lock + snapshot swap + `allSoundOff`).
**Denormals:** `ScopedNoDenormals` present. **Exceptions/dynamic_cast:** none in the audio path (the `dynamic_cast` in CategorySoundPolicy runs on preset load, message thread).

---

## 11. Voice, note, pitch & timing report

**Correct today:** chromatic categories (Leads/Brass/Strings/Synths/Bells) resample by `semitoneRatio(note − root)` with root from smpl chunk > preset > name inference; `PitchAlignmentTests` pass; MIDI note changes pitch immediately; glide ramps per sample.

**Wrong or misleading:**
1. **Tempo sync repitches phrases.** There is no time-stretch. `timeRatio` multiplies the read increment (SamplerEngine.cpp:507), so any BPM sync/speed change shifts pitch. UI offers "Time Stretch" mode that is identical to "Phrase". *This is the single biggest product-level dishonesty in the plugin.*
2. **Phrase sync assumes 4 beats.** `targetSec = beatSec * 4.0` (SamplerEngine.cpp:489) — every phrase window is stretched(=repitched) as if it were exactly one bar; `SRC_ORIGINAL_BPM` is ignored whenever a phrase window exists. An 8-bar phrase at 140 BPM syncs wildly wrong.
3. **Keytrack toggling** (`SRC_KEYTRACK`, default off) is the reason "keys don't change pitch until you toggle something" in phrase categories. The policy layer (`applyPlaybackPolicyToApvts`) resets it per preset — user enables keytrack, switches preset, it silently reverts.
4. **Low-note bleed**: one-shots in non-chromatic categories get `TriggerToEnd` + ignore note-off/all-notes-off; long 808-ish material rings into the next note; in Gated phrase mode `RetriggerPolicy::PhraseChoke` covers same-phrase retriggers but different notes stack.
5. **Recommended target design (v1):** two user-visible modes — **KEYS** (chromatic resample, gated, release env) and **PHRASE** (original pitch/speed, choke-on-new-note, optional loop). Hide speed/BPM-sync until a stretch engine (or clearly label "affects pitch"). One "Root Note" readout with override.

---

## 12. Parameter & preset-state report

Full 249-row table is impractical here; the categories:

| Group | Count | Automated? | Saved/Restored | DSP | Verdict |
|---|---|---|---|---|---|
| Core (gains, reverse, glide, smear, tone, reverb, width→brightness, attack, release, pan) | 12 | yes | yes | yes | OK (pan math broken C2; `stereo_width` ID renders as "Brightness", `smear` as "Filter" — documented compat hack, keep) |
| FX (reverb on/damp, delay×5, chorus×4, lofi×2, dist×2, fx_edits_on) | 17 | yes | yes (FX-lock can strip) | yes | OK |
| Voice (polyphony, play mode, glide mode, velocity sens, amp decay/sustain, limiter) | 7 | yes | yes | yes | OK |
| SRC_* source engine | 11 | yes | yes | yes | Works; model issues §11; policy layer overwrites user values on preset load |
| Chop master + 80 step params | 88 | yes | yes | broken (C4) | Fix or hide |
| PTEX_* texture | 9 | yes | yes | buggy (C5/C6) | Fix |
| PERF_MODE + 8 PERF_FX_* | 9 | yes | yes | 3 broken, 2 no-UI, rest crude | Cull |
| Macros | 4 | yes | yes | yes | OK; defaults 0.5 = mapped center |
| LFO (15), Mod (32), Osc (13), source_blend (1), Filter+FltEnv (10), TEX_* (15), PHRASE_* (8) | 94 | yes (pointlessly) | yes | **none** | Keep IDs (compat), hide everywhere, exclude from user presets |

Notes: parameter *defaults* are mostly sensible; `ENV_ATTACK` default 0 ms conflicts with the schema comment (~5 ms) and causes clicks (§8.1). `kExpectedApvtsParamCount = 249` is a fragile magic number — adding any parameter silently changes partial-preset detection; derive it from the layout instead.

---

## 13. UX & workflow report (first-session producer)

- **First open:** a cockpit photo with ~30 hotspots labeled THROTTLE/HYD/CAB/V1/EMER/TAXI. The plugin's purpose (play sampled instruments) is discoverable only via the preset bar. **No on-GUI keyboard**, no waveform display (M4 TODO admits this), no indication which knob is glide vs. attack until hovering.
- **Workflows** (selected):
  1. *Load & play*: works; preset switch during held notes is safe (choke) but `suspendProcessing` causes a dropout — acceptable, undocumented.
  2. *Adjust envelope*: ATK/REL knobs exist (dash + gauges); decay/sustain only as unlabeled Advanced cells. OK.
  3. *Tone/filter*: three overlapping controls (TONE, FILT/smear, BRT/stereo_width) with aviation names. Confusing but functional.
  4. *Texture layer*: Advanced → TEXTURE ON + MIX — currently sounds wrong (C5/C6).
  5. *Phrase to tempo*: BPM SYNC repitches (§11) — the core disappointment for a phrase-based product.
  6. *Modulate a parameter*: impossible (no DSP), yet knobs show mod rings. Actively harmful.
  7. *Save preset*: silent overwrite, no feedback, favorites vanish on close.
  8. *Recover from missing sample*: sine beep (masked error).
- **Feedback gaps:** no loading states, no error surfaces, no A/B compare, no section reset, no tooltips on the photo controls (labels are 2–4 letter codes).

---

## 14. Performance report

- **Audible/stability-critical:** RT1–RT4 (§10).
- **Meaningful:** cache `getRawParameterValue` pointers (≈150 string-hash lookups per block outside the chop 80); skip `readBaseState` chop-step loop when `CHOP_ON` off; per-sample `updateVoicePlaybackRates` during glide (exp per sample per voice) → update every 32 samples; `getPresetsForCategory` re-scans the filesystem on every browser query + `buildFlatPresetList` rebuilt per navigation.
- **Minor:** double WAV decode on preset load (probe then real — drop the probe, validate once); `EngineState` double copy per block; `constantPowerPan` `sin/cos` per block fine.
- **Avoid (premature):** replacing linear interpolation with Hermite (memory research note) — do it only after the blockers; SIMD voice loops.

## 15. Host-compatibility risks

1. **FL Studio variable block sizes** → RT4 allocation path; also `PlaybackProbe`/blockCount logging (debug).
2. **Ableton transport stop** sends All-Notes-Off → one-shots keep ringing (§8.3).
3. `setStateInformation` before `prepareToPlay` — handled (isPrepared guard) ✓.
4. Repeated editor open/close: the `onPresetLoaded` **callback-chaining** between processor/editor/MainPanel (function objects wrapping each other, restored in destructors) is order-dependent and will break the day a second observer is added → replace with a small listener list on PresetManager.
5. Plugin-scan cost: constructor loads + decodes a preset & WAV synchronously; fine (~ms) but measurable across 10 instances.
6. `PLUGIN_NAME "Aviation"` vs artefacts named both `Aviation` and `AviatorKeyz` in build tree — confirm the shipped bundle identity is final; changing later breaks sessions (VTS3 class ID from PLUGIN_CODE `Avk1`).
7. `known-host-issues.md` sanitizer lesson already institutionalized ✓.

---

## 16. Recommended simplified product structure

**Main page (cockpit, keep the art, rename the words):**
Preset bar (category · prev/next · search · save-as with dialog · favorite) + 8 controls: **Attack, Release, Tone, Filter (HPF), Reverb, Space (size), Air (brightness), Glide** + Volume lever + Reverse toggle + 4 macro knobs with visible target hints. One control per parameter. Remove: TAXI/CRUISE/CLIMB/DESC, V1, EMER, HYD/CAB toggles, duplicate levers/faders.

**Advanced page:**
- *Source*: Mode (Keys | Phrase | One-Shot), Root, Tune, Start/End, Loop, Reverse. (Speed/BPM-sync return with a stretch engine.)
- *Space & FX*: reverb/delay/chorus/lofi/dist as today.
- *Texture*: after C5/C6 fixes — On, Mix, Grain, Density, Position, Width, Freeze.
- Remove tabs/params for: synth, filter, LFO, mod matrix, chop (until C4 + step UI), perf FX (until redesigned as momentary pads).

## 17. Prioritized remediation plan
(P = priority, C = confidence, all "before release" unless noted)

1. **Delete AgentDebugLog + all call sites** — P: blocker, C: confirmed. Risk: none. Test: grep clean; block-perf smoke.
2. **Fix pan law** (`halfPi*0.5f`) — blocker, confirmed. Risk: audibly changes all output (to correct). Test: unit test L==R at center, −3 dB law; null-test hard L/R.
3. **RT allocations**: cache chop-step atomics; ToneShaper in-place coefficients (e.g., precomputed table over tone ∈ [−1,1] ×64 steps or `setCoefficients` on stack), remove per-block `setSize` (clamp instead) — blocker, confirmed. Test: run with allocation-detecting allocator (or `JUCE_STRICT_REFCOUNTEDPOINTER`+custom asserts) under automation sweep.
4. **Envelope click fixes** (min attack ramp, init decay segment, fade on mono steal/legato reuse, honor allNotesOff in TriggerToEnd or fade tails on transport stop) — high, confirmed. Test: record retriggers, assert no discontinuity > x dB between samples; extend VoiceLifecycleTests.
5. **Hide dead surface**: source_blend/synth, filter, LFO, mod rings (PrecisionKnob), "Time Stretch" choice, dead cockpit buttons, tape-stop/pitch-drop/scatter/sweep — high, confirmed. Keep parameter IDs registered (compat). Test: UI walkthrough; StateSchemaTests unchanged.
6. **Fix texture (C5, C6)** — high, confirmed. Test: grain null at mix=0, monotonic loudness vs mix, spectrogram click check.
7. **Fix or defer chop (C4)** — high, confirmed. If kept: sample-domain clock + intra-block step boundaries + real gain smoothing; else hide CHOP cells.
8. **Preset UX**: save-as dialog + overwrite confirm + persist favorites (properties file) + `setValueNotifyingHost` in restoreFxParams — medium, confirmed.
9. **Phrase/BPM honesty**: remove 4-beat assumption; use SRC_ORIGINAL_BPM; label speed "affects pitch"; keytrack persistence across preset load — medium-high.
10. **Cleanups**: dead members (filterProcessor, textureEngine, synthScratch), double `fxChain.reset()`, `processBlockBypassed` reads, `_legacy` + 8 dead Advanced components out of the build, stale ARCHITECTURE.md (64 voices) & TODO.md ("mod matrix ✓") — medium, low risk.
11. **macroControls race** → atomic snapshot pointer — medium, likely.
12. Later: stereo sample support (SampleLibrary currently downmixes everything to mono — significant quality ceiling for pads/ensembles), Hermite interpolation, real time-stretch, real perf FX, LFO/mod-matrix wiring, pitch bend support (MidiHandler ignores pitch bend & CC1 entirely — notable for a "keys" product).

## 18. Suggested next-release scope
**In:** items 1–9 above; sampler + FX + texture + macros; 10 categories; simplified cockpit.
**Defer:** chop sequencer (unless step UI planned), synth layer, LFO/mod matrix, time-stretch, perf FX pads, stereo samples, pitch bend.
**Remove permanently:** Scatter, per-block Reverse FX, filter-sweep in current form, PERF_MODE enum (keep bools), flight-mode/panic/emergency cockpit buttons.

## 19. Files to change first
1. `Source/Debug/AgentDebugLog.h` (+11 call-site files) — delete.
2. `Source/DSP/FastMath.h` — pan law (:25), hannWindow (:53).
3. `Source/DSP/Performance/PerformanceApvtsReader.{h,cpp}` — cached param pointers.
4. `Source/DSP/ToneShaper.cpp` — allocation-free coefficient update.
5. `Source/PluginProcessor.cpp` — remove per-block setSize, dead engines, agent logs; wire nothing new.
6. `Source/DSP/SamplerEngine.cpp` — envelope/steal fixes (:326, :395, :645, :686).
7. `Source/DSP/Performance/TextureBlendEngine.cpp` — double-dry.
8. `Source/DSP/Performance/PhraseChopper.cpp` — clock units (or hide feature).
9. `Source/GUI/Cockpit/CockpitZones.cpp` — dead/miswired anchors.
10. `Source/GUI/PrecisionKnob.cpp` — remove fake mod display.

## 20. Questions requiring product decisions
1. Is the **synth layer** part of the product vision, or is AviatorKeyz a pure sample instrument? (Decides whether 13 osc params + SynthEngine get wired or deleted from UI for good.)
2. Is the **chop step sequencer** a v1 feature? It needs the clock fix *plus* a 16-step editor UI that doesn't exist.
3. Should **BPM sync** ship as "repitch" (honest, vinyl-style) or wait for time-stretch?
4. Cockpit metaphor: keep aviation *labels* (THROTTLE/TURBULENCE) or aviation *visuals* with musical labels? (Recommend the latter.)
5. Favorites/user presets: per-machine file or inside host state?
6. `stereo_width`→"Brightness" and `smear`→"Filter": accept the ID/name mismatch forever (recommended), or bump schema and migrate?
7. Mono sample pipeline: is losing stereo in pads/ensembles acceptable for v1?

---

## Phased implementation plan

**Phase 1 — Stabilize (1–2 weeks):** items 1–4 + 11 of §17. Exit: no allocation/file-I/O in callback under automation sweep; centered image; clickless retriggers; 112+new tests green.
**Phase 2 — Repair (1–2 weeks):** texture C5/C6; chop C4 *or* hide; preset UX (save-as, favorites, notify-host); keytrack persistence; allNotesOff policy.
**Phase 3 — Simplify (1–2 weeks):** cockpit control diet (§16); hide dead params; honest playback-mode list; macro target visibility; tooltips; delete dead GUI code + stale docs.
**Phase 4 — Optimize (1 week):** cached param pointers everywhere; glide update decimation; preset list caching; single WAV decode; (optional) stereo samples.
**Phase 5 — Validate:** matrix = {FL Studio, Ableton Live} × {44.1/48/96 kHz} × {64/128/512/2048 & FL variable} × {2 instances} × tests: automation sweep on every *visible* param; preset recall + project reopen null-test; offline render == realtime render; MIDI stress (64 notes/s, repeated same-note, sustain, low notes C0–C2); tempo ramp 60→180 BPM during phrase playback; transport loop-jump; sound switching during playback; editor open/close ×50; kill-host recovery. Automate what's automatable in the existing test target — pan-center, window-endpoint, chop-step-sequence, allocation-guard tests are all unit-testable today.
