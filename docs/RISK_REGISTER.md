# AviatorKeyz — Risk Register

Each risk has: description, likelihood (H/M/L), impact (H/M/L), mitigation plan, and owner milestone.

---

## R-01 — Sample streaming latency on large files

**Likelihood:** M | **Impact:** H

**Description:** Large sample files (>50 MB) loaded into RAM may cause `prepareToPlay` to block the audio thread during initial load, causing a dropout or freeze in FL Studio.

**Mitigation:**
- Files below a configurable threshold (e.g. 20 MB) are loaded fully into RAM
- Files above threshold are streamed from disk using `juce::AudioFormatReader` with a pre-fill buffer
- Loading always happens on the message thread — audio thread never reads from file directly
- Pre-fill occurs in `prepareToPlay` on the audio thread only after the buffer is ready via atomic flag

**Owner milestone:** M1 (initial) — M3 (user import)

---

## R-02 — Reverse mode glitches at note boundaries

**Likelihood:** M | **Impact:** M

**Description:** When a voice begins playing backward from the end of a sample, edge conditions at the loop/end boundary (read position < 0) can cause reads outside the buffer, producing clicks or crashes.

**Mitigation:**
- SamplerVoice clamps read position to [0, sampleLength - 1] before every read
- At boundary, voice transitions to release state rather than wrapping
- Crossfade applied at boundary (configurable length, default 10 ms) to mask the cut
- Boundary behavior is covered in test T-M2-02

**Owner milestone:** M2

---

## R-03 — Glide + Reverse interaction

**Likelihood:** L | **Impact:** L

**Description:** Glide (pitch ramp) and Reverse (buffer direction) are conceptually independent, but the interaction could produce unexpected results if both are active and the pitch ratio sign interacts incorrectly with the read direction.

**Mitigation:**
- Spec is clear: Glide affects pitch ratio magnitude only; ReversePlayer applies sign
- `ReversePlayer::getReadIncrement(pitchRatio, reversed)` returns `reversed ? -pitchRatio : pitchRatio`
- These are strictly separate operations applied in sequence — no special case needed
- Covered by test T-M2-01 + T-M2-03 run together

**Owner milestone:** M2

---

## R-04 — FL Studio VST3 recognition failure

**Likelihood:** L | **Impact:** H

**Description:** FL Studio may fail to scan or incorrectly categorize the plugin if VST3 metadata flags are wrong, causing the plugin to appear under Effects instead of Instruments, or not appear at all.

**Mitigation:**
- `IS_SYNTH=TRUE` and `VST3_CATEGORIES="Instrument|Synth"` are set in CMakeLists.txt
- Plugin has no audio inputs (instruments should have no input bus or disabled input)
- `isBusesLayoutSupported()` rejects any layout with an active main input bus
- FL Studio recognition test (T-M0-03) is the first M0 exit criterion — tested before anything else

**Owner milestone:** M0 ← test immediately

---

## R-05 — APVTS parameter ID drift across versions

**Likelihood:** L | **Impact:** H

**Description:** If a parameter ID string is changed between builds, any host that saved a project with the old ID will silently receive the default value instead of the saved value. This permanently corrupts saved projects for existing users.

**Mitigation:**
- All IDs are `constexpr` strings in `Source/State/StateSchema.h` — only place they exist
- The parameter table in `docs/PARAMETERS.md` is the human-readable reference
- CI (M5) will run a build-time check: grep for hardcoded param ID strings outside StateSchema.h and fail if found
- IDs are flagged as CRITICAL in code comments

**Owner milestone:** M0 (prevention) — M5 (CI enforcement)

---

## R-06 — Sample import: unsupported format crashes

**Likelihood:** M | **Impact:** M

**Description:** A user imports an audio file in an unsupported format (MP3, FLAC, OGG, corrupted WAV) and the plugin crashes or produces silence without explanation.

**Mitigation:**
- `SampleLibrary` validates format via `juce::AudioFormatManager::findFormatForFileExtension()` before touching any DSP
- Unsupported format: display a clear error message in the UI, do not crash
- Corrupted file: wrap format reader creation in a null check, fail gracefully
- Only load if `juce::AudioFormatReader` is non-null after open attempt

**Owner milestone:** M3

---

## R-07 — Multiple instance state contamination

**Likelihood:** L | **Impact:** H

**Description:** If any class uses `static` or global state (e.g. a static `LookAndFeel` instance, a global sample cache), opening multiple instances of the plugin may cause them to share and corrupt each other's state.

**Mitigation:**
- No `static` mutable state permitted in DSP, GUI, or State modules
- `LuxuryLookAndFeel` is instantiated per-editor, not globally
- `SampleLibrary` holds no shared cache in v1 — each instance loads independently
- Multi-instance test (T-M5-01) is a required M5 exit criterion

**Owner milestone:** M4 (review) — M5 (test)

---

## R-08 — prepareToPlay not called before sample rate / buffer changes

**Likelihood:** M | **Impact:** M

**Description:** Some hosts call `prepareToPlay` only once at project open and never again when the sample rate changes mid-session. DSP modules that cache sample rate without handling changes may produce wrong pitch or incorrect timing.

**Mitigation:**
- All DSP modules accept and store `sampleRate` and `blockSize` in `prepare()`
- `processBlock` does not cache these values directly — it reads from the module's prepared state
- Transport restart and sample rate change are both covered in test T-M1-07 and T-M5-02

**Owner milestone:** M1

---

## Active Monitoring

Risks R-04 (FL Studio scan) and R-05 (parameter ID drift) are the two highest-priority risks to catch early. Both must be verified at M0 completion before any M1 work begins.
