# SamplerEngine — Architecture Spec (M1)

**Status:** Draft v0.1 — 2026-05-19
**Owner:** DSP layer
**Source files:** `Source/DSP/SamplerEngine.{h,cpp}`, `Source/DSP/ReversePlayer.{h,cpp}`, `Source/DSP/GlideEngine.{h,cpp}`
**Consumes:** `Source/State/SampleLibrary.{h,cpp}` (M1+), APVTS via `PluginProcessor`
**Authoritative parent doc:** [`docs/ARCHITECTURE.md`](ARCHITECTURE.md) — this file refines its "Voice Architecture (M1)" section.

---

## 1. Purpose

`SamplerEngine` is the polyphonic voice pool that turns MIDI note events into audio. It owns voice state, voice allocation, per-voice playback (pitch, position, direction, envelope), and the final per-block mix into the processor's output buffer.

It is the only DSP module that touches MIDI note state. Everything downstream (`ToneShaper`, `SmearProcessor`, `ReverbTail`, width/pan) operates on the mixed sample-engine output and knows nothing about notes or voices.

---

## 2. Responsibilities

In scope:

- Maintain a fixed pool of voices, pre-allocated at construction.
- Translate MIDI note-on/off into voice allocation, voice-stealing, and release-stage transitions.
- Per-voice playback of a sample buffer with pitch ratio, read direction (forward/reverse), linear interpolation, and an amplitude envelope.
- Sine-wave fallback when no sample is loaded — so the plugin always makes sound during early development and during sample-load races.
- Glide (portamento) between successive note-ons.
- Mix all active voices into a stereo output buffer, summing into whatever the caller has already placed there (additive — never clear).

Out of scope (handled elsewhere):

- File I/O, format parsing, sample-rate conversion of source files — `SampleLibrary`.
- Parameter management, smoothing, APVTS reads — `PluginProcessor`.
- MIDI parsing, sustain pedal bookkeeping, CC routing — `MidiHandler`.
- Tone shaping, reverb, smear, width, pan, output gain — downstream DSP modules.
- Waveform rendering, playhead display, GUI state — GUI layer (read-only feedback via atomics if needed, not yet wired).

---

## 3. Threading Contract

This is the most load-bearing section. Violating it causes glitches, crashes, or undefined behavior under host stress.

| Method | Caller | Allowed to allocate? | Allowed to block? | Notes |
|---|---|---|---|---|
| `SamplerEngine()` / `~SamplerEngine()` | message thread | yes | yes | Construction only — no audio running. |
| `prepare(spec)` | message thread, between `prepareToPlay` and resume | yes | yes | Audio is paused by the host. |
| `releaseResources()` | message thread | yes | yes | Audio is paused. |
| `setSampleTable(...)` | message thread or message-thread-equivalent prep phase | **no** | **no** | Must be paired with audio-thread quiescence (see §4). |
| `setEnvelopeTimesMs(...)` | message thread | no | no | Atomic-equivalent writes only; safe to call while audio runs in practice but prefer prep phase. |
| `noteOn / noteOff / allNotesOff / allSoundOff` | audio thread (called from `processBlock` via `MidiHandler`) | **no** | **no** | RT-safe; must complete in bounded time. |
| `process(buffer)` | audio thread | **no** | **no** | Hot path. No locks, no atomics in inner loop, no virtual calls per sample if avoidable. |

**Key invariant:** the audio thread never reads a sample buffer pointer that is being mutated. `setSampleTable` either runs while audio is paused (preferred for M1) or — once the multi-region path lands in M2 — uses a double-buffer + atomic-pointer swap so the audio thread always sees one complete, immutable snapshot.

**Today (M1 in progress):** the implementation stores `const float* sampleData` directly. The contract is "only set from message thread between blocks." That is fragile under FL Studio's hot-reload / drag-load flow; M1 must upgrade this to the double-buffer swap before merging.

---

## 4. Sample-Table Lifecycle

Today's interface accepts a single mono buffer plus one root note:

```cpp
void setSampleTable (const float* monoSamples, int numFrames, int rootMidiNote);
```

`PluginProcessor::loadFactorySample()` does the work: decodes embedded `BinaryData` via `juce::AudioFormatManager`, sums to mono into `factoryMono` (a `HeapBlock<float>` owned by the processor), and hands the pointer to `SamplerEngine`. The processor owns the buffer's lifetime; the engine only holds a non-owning pointer.

This is fine for M1's "one sample, one root note" target but **does not** yet consume `SampleLibrary::getSampleMap()`, which already models multiple `SampleRegion`s with `noteMin/noteMax/velocityMin/velocityMax`. The bridge is the central M1 deliverable.

### Proposed M1.5 interface

```cpp
// Replaces setSampleTable. Engine takes a snapshot pointer + frame count
// and the region selection logic moves into the engine (or a small helper).
void setSampleSnapshot (std::shared_ptr<const SampleSnapshot> snap) noexcept;
```

Where `SampleSnapshot` is an immutable struct built on the message thread holding:

- A flat array of `Region { const float* data; int frames; int rootNote; uint8_t loKey, hiKey; float loVel, hiVel; }` — pointer-to-data lives in a buffer that `SampleSnapshot` keeps alive.
- An optional lookup table `regionForNote[128]` (or `regionForKey[128][N_velLayers]`) pre-computed so voice allocation is O(1) on the audio thread.

The audio thread atomically loads `std::atomic<std::shared_ptr<const SampleSnapshot>>` once per `process()` call (not per sample), drops to a raw pointer for the inner loop, and the old snapshot is freed on the message thread when its refcount hits zero. **No `shared_ptr` ref-bump on the audio thread inside the per-sample loop.**

Open question for M1: stick with raw `setSampleTable + pause` for the first cut and ship M1; promote to `SampleSnapshot` as M1's final commit before opening M2. Flag in [`TODO.md`](../TODO.md).

---

## 5. Voice Model

```cpp
struct Voice {
    bool     active;
    int      noteNumber;
    float    velocity;          // 0..1, baked into output gain
    float    phase;             // sine fallback only
    float    readPos;           // fractional read index into sample buffer
    bool     reversed;          // latched at note-on; mid-note flip not supported
    float    currentPitch;      // float MIDI note, glides toward target
    float    targetPitch;
    float    glideIncPerSample; // signed delta toward target per sample
    bool     gliding;
    EnvStage envStage;          // idle | attack | sustain | release
    float    envLevel;          // 0..1, linear amplitude
    float    envLinearStep;     // per-sample delta during attack/release
    int      envSegSamplesLeft; // segment countdown
};
```

### Pool size

- Code today: `kMaxVoices = 16`.
- M1 target per `MILESTONES.md` and `ARCHITECTURE.md`: **64**.
- M1 acceptance only needs 8 simultaneous voices without stealing, so 16 currently passes the gate. **Do not** raise to 64 silently — bump it deliberately when the SamplerEngine work is otherwise stable, in its own commit, with a note in `CHANGELOG.md`. Voice count affects worst-case CPU and is observable in tests.

### Voice allocation

`findFreeOrStealVoice()`:

1. Linear scan for `active == false`. Return first hit.
2. Otherwise steal the voice with the lowest `envLevel` (quietest). Linear scan, O(N), N=16.

This works because all voices are at most `kMaxVoices` and we sweep them every block anyway. Avoids the priority-queue machinery and stays branch-predictable.

### Voice stealing edge cases

- Stolen voice does **not** crossfade. M1 acceptance does not require it; many existing samplers click on theft. Add to risk register if it shows up in FL Studio testing.
- Steal target may already be in release. Restarting it as attack from current `envLevel` (instead of from 0) avoids a click. The current `startVoice` resets `envLevel = 0` and ramps from there — fine in practice because attack times start at 0.5 ms; but if attack is set to 0.5–1 ms and theft hits a voice still at level 0.8, the discontinuity is audible. Track in `docs/RISK_REGISTER.md`.

---

## 6. Per-Voice Playback

### 6.1 Pitch

```
ratio = 2^((currentPitch - rootMidiNote) / 12)
```

`currentPitch` is a float; the integer note number is only used for note-off matching. Glide ramps `currentPitch` toward `targetPitch` once per rendered sample, so pitch and read-position stay phase-coherent.

### 6.2 Read position + interpolation

Linear interpolation between adjacent frames. Read direction is latched at note-on (`v.reversed`) and never flipped mid-voice. The buffer is treated as a one-shot with wraparound at the boundary opposite the direction of travel:

- Forward: when `readPos >= numFrames - 1`, subtract `numFrames - 1`. Effectively a loop point at the tail.
- Reverse: when `readPos <= 0`, add `numFrames - 1`. Loop point at the head.

This is wrong for samples that should one-shot and stop (drums, plucks with no tail). M1 doesn't need it; M2 introduces sustain/loop metadata in `SampleRegion`. Until then, releases cut the tail via envelope, which is acceptable.

**M2 upgrade:** higher-quality interpolation (Hermite or cubic, JUCE has `WindowedSincInterpolator`) is a separate, gated decision — benchmark first, the linear path is currently inaudible for the source material.

### 6.3 Reverse

Lives in `ReversePlayer::getReadIncrement(ratio, reversed)`. Today this is trivial (return `±ratio`), but isolating it keeps Reverse logic in one place and lets the unit test for "reverse should preserve speed magnitude" live next to it.

The `reverse` APVTS param is read by `PluginProcessor` and passed into `noteOn()` per-note. **Toggling the param during a held note has no effect until the next note-on** — this is deliberate: flipping read direction mid-stream causes a click, and per-voice reverse only sounds right when the buffer head/tail position is established at note-on.

If the user expects "live" reverse, we can add a crossfade in M2.

### 6.4 Glide

Glide is **note-to-note**, not voice-to-voice:

- `lastNoteForGlide` is shared across the engine.
- On `noteOn(...)`, if `glideTimeMs > 1` and `lastNoteForGlide >= 0`, the new voice starts at `currentPitch = lastNoteForGlide` and ramps to `targetPitch = midiNote` over `glideTimeMs`.
- `lastNoteForGlide` is updated to `midiNote` whether or not glide engaged.

This is "always-on legato glide" — every note glides from the previous one. Some samplers gate glide on overlap; FL Studio users typically expect "always glide". Worth confirming with target users; trivial to switch later by gating on "any other voice active."

`glideIncPerSample` is set at note-on; sample-rate changes during the glide are not handled. M1 acceptance includes "no artifacts on sample rate change" — verify this corner.

### 6.5 Envelope

Today: linear AR with hold-at-1.0 sustain. No D, no S level.

- **Attack:** linear ramp 0 → 1 over `attackMs`.
- **Sustain:** held at 1.0 until note-off.
- **Release:** linear ramp from `envLevel` → 0 over `releaseMs`, from whatever level we were at when note-off arrived.

Linear amplitude release sounds unnaturally fast at the start and overstays at the end. Acceptable for M1, but `RISK_REGISTER.md` should carry "release tail feels artificial — exponential or 60-dB log curve in M2."

Decay + sustain-level promotion (full ADSR) is M2 work driven by APVTS params `env_decay` and `env_sustain` — those param IDs **don't exist yet** in `StateSchema.h`. When added, append per the rules in `docs/PARAMETERS.md` and bump nothing (additive change).

### 6.6 Per-sample inner loop summary

For each active voice, each sample:

1. Advance glide (cheap branch — skipped if not gliding).
2. Compute `pitchRatio` from `currentPitch - sampleRootNote`.
3. Linear-interp read; advance `readPos`; wrap.
4. Multiply by `velocity * envLevel`.
5. Advance envelope; check stage transitions.

No allocations, no atomics, no virtual calls, no `std::function`, no exceptions. `std::pow` per-sample for the pitch ratio is the costliest single op — fine for 16 voices at 96 kHz, but a candidate for pre-baking when scaling to 64 voices. Track in `docs/RISK_REGISTER.md`.

---

## 7. Output Mixing

`process(buffer)` is **additive**: it sums into whatever's in `buffer`. The processor's chain order is responsible for clearing or pre-filling appropriately. SamplerEngine's contract: "I add my sound; I don't touch what's already there."

Mono-sums to both channels for now. Stereo sample layers, key-position pan, and per-voice pan are M2+.

---

## 8. APVTS Touch Points

These are the parameters the engine cares about (read by `PluginProcessor`, written into the engine):

| APVTS ID | How it reaches the engine | Notes |
|---|---|---|
| `reverse` | bool arg into `noteOn(... reverse ...)` | Latched at note-on. |
| `glide_time` | float-ms arg into `noteOn(... glideTimeMs ...)` | Skewed range 0–500 ms; 0 = off. |
| `env_attack` | `setEnvelopeTimesMs(att, ...)` | Clamped 0.5–5000 ms. |
| `env_release` | `setEnvelopeTimesMs(..., rel)` | Clamped 5–10000 ms. |

`input_gain`, `output_gain`, `pan`, `stereo_width`, `tone`, `smear`, `reverb_*` are downstream of the engine and the engine does not see them.

**Parameter ID stability:** all of the above IDs are frozen in `Source/State/StateSchema.h`. Never reference them by string literal anywhere outside that header — always `AviatorKeyz::ParamID::REVERSE` etc.

---

## 9. MIDI Touch Points (via `MidiHandler`)

`MidiHandler` is the audio-thread component that walks the `juce::MidiBuffer` for each block and calls into `SamplerEngine`. Required hooks for M1:

- **Note-on (vel > 0):** `samplerEngine.noteOn(note, vel/127.0f, reverse, glideMs)`
- **Note-off / note-on(vel=0):** `samplerEngine.noteOff(note)`
- **All notes off (CC 123):** `samplerEngine.allNotesOff()`
- **All sound off (CC 120):** `samplerEngine.allSoundOff()`
- **Sustain pedal (CC 64):** held in `MidiHandler`. While pedal is down, note-offs are buffered and not forwarded to the engine until pedal release. **The engine itself has no sustain bookkeeping** — keep it that way; sustain is a host-controller concern, not a voice-state concern.

---

## 10. Sample-Rate / Buffer-Size Changes

Host can call `prepareToPlay` at any time. Required engine behavior:

1. `prepare(spec)` updates `sampleRate`. **Active voices are torn down** via `allSoundOff()` to avoid stale `glideIncPerSample` and `envLinearStep` values that were computed against the old rate. This is a clean break — no attempt to rescale ongoing ramps.
2. Buffer size has no direct effect on `process()` correctness because everything is per-sample, but pre-allocations sized to `samplesPerBlock` (if any are added later) must be redone.

M1 acceptance "no artifacts on sample rate / buffer size change" can be satisfied with the tear-down approach. A "no audible click on rate change" upgrade is M5 polish.

---

## 11. Determinism & Bypass

- The engine's per-sample math is fully deterministic given the same MIDI input and parameter values — required for offline render parity.
- `PluginProcessor::processBlockBypassed` must not call `samplerEngine.process()`. It should still drain `MidiHandler` to keep note state consistent on unbypass (FL Studio's transport can deliver MIDI to bypassed plugins). Spec for bypass behavior on unbypass: voices that would be playing if not bypassed are **not** started — bypass is a hard mute, not a "process and dump."

---

## 12. Test Hooks

Unit-testable surfaces (no JUCE host needed; link against juce_audio_basics + juce_dsp only):

1. **Voice allocation:** trigger 17 note-ons on a 16-voice engine; assert exactly one steal and that it stole the quietest.
2. **Note-off → release:** note-on, advance N samples, note-off, advance `releaseMs * sampleRate` samples, assert `envLevel == 0` and `active == false`.
3. **Glide ramp:** note-on(60) → note-on(72) with 100 ms glide; sample `currentPitch` per-block and assert monotonic ramp from 60 to 72 in exactly `sampleRate * 0.1` samples ± rounding.
4. **Reverse read position:** load known 1000-frame buffer, note-on with reverse=true, render 100 samples, assert `readPos` decreased from 999 by `100 * ratio`.
5. **Sample-rate change:** start a voice, call `prepare(48kHz)` then `prepare(96kHz)`, assert engine is in clean state (no active voices, no nonzero envelope).
6. **No-allocation:** wrap `process()` in a malloc-trap (override `operator new`) and run 10,000 blocks of random MIDI. Assert zero allocations.

`TESTPLAN.md` should reference this section by number when checking M1 exit criteria.

---

## 13. Known Limitations (carry into `RISK_REGISTER.md`)

| Risk | Severity | Mitigation |
|---|---|---|
| Linear release curve sounds artificial | Low | M2: exponential / log-amplitude release. |
| Voice theft restart-from-zero can click | Low | M2: restart from prior `envLevel` or short crossfade. |
| Mid-note reverse toggle ignored | Low (spec'd) | Document in UI tooltip. |
| Glide ignores sample-rate changes mid-glide | Low | Tear-down on `prepare()` handles it; verify test passes. |
| `std::pow` per-sample for pitch ratio | Medium at 64 voices | Cache ratio per voice; refresh only when `currentPitch` changes. |
| Single mono sample buffer (no map yet) | High for M1 completion | M1.5 `SampleSnapshot` work. |
| Raw `const float*` swap is fragile under hot-reload | High | M1.5 double-buffer + atomic-pointer or `shared_ptr<const Snapshot>`. |
| `kMaxVoices = 16` vs spec'd 64 | Medium | Raise deliberately with a CPU test in its own commit. |

---

## 14. M1 Exit Checklist (engine-scoped)

Mirrors `MILESTONES.md` § M1 with engine-level granularity:

- [ ] `setSampleSnapshot` (or interim equivalent) consumes `SampleLibrary`.
- [ ] 8 simultaneous note-ons play with no stealing observed.
- [ ] Sustain pedal holds notes (verified via `MidiHandler` integration).
- [ ] All-notes-off / all-sound-off stop voices.
- [ ] Sample-rate change at 44.1 → 48 → 96 kHz produces no artifacts, no crashes, voices torn down cleanly.
- [ ] Buffer size 256 → 512 → 1024 produces no artifacts.
- [ ] Malloc-trap test passes (zero allocations in `process()`).
- [ ] `kMaxVoices` decision made and recorded (stay at 16 for M1 closeout or raise to 64).
- [ ] Voice allocation + envelope unit tests passing.

---

## 15. Open Questions

1. **Glide gating** — always-on legato vs. only-when-overlapping? Default proposal: always-on.
2. **Voice count for M1 ship** — 16 (current) or 64 (spec)? Default proposal: ship M1 with 16, raise to 64 in M2 polish after CPU profiling.
3. **Snapshot promotion timing** — do the `SampleSnapshot` upgrade inside M1 or as the first commit of M2? Default proposal: inside M1, because FL Studio drag-load is a realistic hot-reload path.
4. **Interpolation quality** — keep linear or move to Hermite? Default proposal: linear through M1; revisit only if M2 listening tests flag aliasing.

Resolve these before closing M1.
