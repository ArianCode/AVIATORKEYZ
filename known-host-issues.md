# AviatorKeyz — Known Host Issues

This file documents host-specific behavior, workarounds, and compatibility notes.
Core plugin code should never contain host-specific hacks — all workarounds are isolated here for review.

---

## macOS

### MAC-001 — VST3 install location
**Status:** Informational  
**Issue:** macOS hosts only scan standard VST3 folders.  
**Steps:**
1. Build: `./scripts/build_macos.sh install` (or copy manually)
2. Confirm bundle at `~/Library/Audio/Plug-Ins/VST3/AviatorKeyz.vst3`
3. Rescan plugins in the DAW  
**Notes:** System-wide path `/Library/Audio/Plug-Ins/VST3/` requires admin; prefer user folder for development.

### MAC-002 — Gatekeeper / unsigned builds
**Status:** Informational  
**Issue:** Unsigned debug builds may be blocked on first launch.  
**Mitigation:** Ad-hoc sign locally, or allow in **System Settings → Privacy & Security**. Release builds require Developer ID + notarization — see `docs/CODE_SIGNING.md`.

### MAC-003 — Standalone vs VST3 state paths
**Status:** Informational  
**Issue:** Standalone and DAW instances use the same user preset folder (`~/Documents/AviatorKeyz/Presets/`).  
**Mitigation:** No host-specific hacks in plugin code; document path for QA.

---

## FL Studio

### FLSI-001 — Plugin category assignment
**Status:** Verified working at M0
**Issue:** FL Studio uses VST3_CATEGORIES to determine whether a plugin appears under Instruments or Effects.
**Requirement:** Must be `"Instrument|Synth"` — not `"Instrument"` alone, not `"Synth"` alone.
**Location:** `CMakeLists.txt` → `VST3_CATEGORIES "Instrument|Synth"`
**Notes:** Verified this causes AviatorKeyz to appear under Instruments in FL's browser.

### FLSI-006 — ASan-instrumented VST3 aborts FL Studio during plugin load
**Status:** Binary fix verified (2026-06-14); FL Studio runtime validation pending  
**Issue:** The AviatorKeyz VST3 was compiled and linked with AddressSanitizer and loaded into an already-running, non-ASan FL Studio process. During dynamic initialization, the ASan runtime attempted to verify its allocator and libc interceptors. In this host and loading configuration, the required interceptors were not established correctly, so `__sanitizer::VerifyInterceptorsWorking()` deliberately called `abort()` before normal AviatorKeyz initialization began.

**Verified facts**
- ASan interceptor verification aborted during `dlopen`.
- The production VST3 contained `libclang_rt.asan_osx_dynamic.dylib` and sanitizer symbols.
- The installed bundle had been copied from a Debug `build-asan/` tree built when root `CMakeLists.txt` applied global `-fsanitize=address,undefined` to all Debug targets.

**Inference (not universally proven)**
- Loading the sanitizer-instrumented bundle into the unsanitized FL Studio process caused the abort in this configuration.
- This does **not** prove that all ASan-instrumented audio plugins fail in every host.

**Fix / workflow**
1. Use `./scripts/build_macos.sh install` (Release, sanitizers OFF) for DAW testing.
2. Debug plugin builds remain host-loadable and sanitizer-free: `./scripts/build_macos.sh debug`
3. Sanitizer unit tests only: `./scripts/build_macos.sh sanitizer-tests` (`-DAVIATORKEYZ_BUILD_PLUGIN=OFF`, artefacts stay in `build-asan/`)
4. Verify any candidate binary: `./scripts/verify_plugin_binary.sh ~/Library/Audio/Plug-Ins/VST3/AviatorKeyz.vst3/Contents/MacOS/AviatorKeyz`
5. Complete FL Studio checklist: `docs/FL_STUDIO_VALIDATION.md` (host: **FL Studio 25.2.3.4889** on macOS 15.7.4 — see `docs/HOST_TEST_CONFIG.md`)

**Notes:** Do not set `ASAN_OPTIONS` or disable interceptor verification as a workaround. Remove sanitizer instrumentation from host-loaded plugin binaries.

### FLSI-002 — FL Studio plugin scan path
**Status:** Informational
**Issue:** FL Studio does not automatically pick up new VST3s — user must initiate a scan.
**Steps:**
1. Copy `AviatorKeyz.vst3` to `C:\Program Files\Common Files\VST3\`
2. FL Studio → Options → Manage plugins → Find more plugins → Start scan
**Notes:** FL Studio caches plugin info. If the plugin changes significantly (bus layout, name), delete FL's plugin cache and rescan.

### FLSI-003 — FL Studio automation clips and parameter IDs
**Status:** Informational — critical for M3+
**Issue:** FL Studio stores automation by parameter ID string. If a parameter ID changes between builds, existing automation clips become orphaned silently.
**Mitigation:** Parameter IDs in `StateSchema.h` are fixed. Never change them.

### FLSI-004 — FL Studio and multiple plugin windows
**Status:** To be tested at M5
**Issue:** FL Studio may share the same editor instance across multiple channel references in some configurations.
**Mitigation:** `AviatorKeyzEditor` holds a reference to its own processor only. No static/global GUI state.

### FLSI-005 — FL Studio transport position
**Status:** To be investigated at M2
**Issue:** FL Studio does not always call `prepareToPlay` when transport loops back to start — it may continue with the existing audio context.
**Mitigation:** Ensure voice states reset cleanly on transport stop. Watch for stuck notes on loop restart.

### FLSI-008 — Reopened project (or offline render) played a sine instead of the preset sample
**Status:** Fixed in code (2026-09-02); FL Studio runtime validation pending
**Issue:** On reopening a saved `.flp` — and on starting an offline render — the plugin
restored its preset name and every parameter correctly, but played a bare sine tone instead
of the loaded sample.

**Root cause (host lifecycle, not FL-specific):** hosts call
`setStateInformation` → `releaseResources` → `prepareToPlay` in that order.
`SamplerEngine::releaseResources()` nulls `sampleSnapshot`. `prepareToPlay` then only
reloaded when `sampleId != loadedSampleId` — but `setStateInformation` had already set
`loadedSampleId` to the restored id, so the reload was skipped and the engine came up with
no snapshot attached. `SamplerEngine::startVoice` falls back to a sine oscillator when no
region is available (`Source/DSP/SamplerEngine.cpp`), which is why the symptom was a wrong
sound rather than silence.

**Fix:** `prepareToPlay` now keys off `SamplerEngine::hasLoadedSample()` rather than the id
comparison. If the snapshot is missing but `SampleLibrary` still owns the published buffer,
the pointer is re-attached with no decode; only a genuinely lost buffer triggers a full
`loadFactorySample`. `loadFactorySample`'s same-sample early-return does the same re-attach.

**Related:** `hasLoadedSample()` was split out of `validateCurrentState()`. The old
combined check treated `amp sustain == 0` as invalid state, so a legitimate percussive
patch reported a failed load and forced a full WAV re-decode inside `prepareToPlay` on
every transport start. Zero sustain is now logged, not failed.

**Regression tests:** `tests/HostStateRoundtripTests.cpp` drives the real
`AviatorKeyzProcessor` (test target builds it with `AVIATORKEYZ_HEADLESS_TESTS=1`) through
the host lifecycle. Note that a peak/silence check alone does **not** catch this — the sine
fallback is loud. The load-bearing assertion is `hasSamplerSampleForTest()`.

**Still to validate in the host:** save a project in FL, close, reopen, confirm the loaded
sound plays; then bounce/render the same project and confirm the rendered audio matches.

### FLSI-007 — Prototype #1 multi-version FL matrix (Windows)
**Status:** QA pending — see `docs/PROTOTYPE1_FL_MATRIX.md`
**Issue:** Prototype #1 must be validated independently on each FL version the client uses. All **64-bit** hosts (FL 11.1+, 20, 21+, 25) share one **x64** `Aviation.vst3` at `C:\Program Files\Common Files\VST3\`.
**Requirements:**
1. Create a **native empty project** in each FL version under test — do not reuse `.flp` files across versions.
2. A project saved in a **newer** FL cannot open in an **older** FL (Image-Line project version rule).
3. **FL 25 on Windows** is a valid test host; do not substitute FL 25 **macOS** `.flp` files for Windows FL 11/20 testers.
4. Tag failures in this file as **FL11**, **FL20**, **FL21+**, or **FL25** (with exact build) to separate legacy-wrapper issues from packaging/runtime bugs.
**Notes:** FL 11 32-bit remains the only case requiring a separate **x86** VST3 build (`scripts\build_windows_x86.bat`).

---

## General VST3 Notes

### GVST-001 — VST3 bundle structure (macOS)
On macOS the binary lives inside the bundle:
```
AviatorKeyz.vst3/
└── Contents/
    └── MacOS/
        └── AviatorKeyz
```
JUCE's CMake build creates this structure automatically. Do not manually rename the binary.

### GVST-001b — VST3 bundle structure (Windows)
On Windows:
```
AviatorKeyz.vst3/
└── Contents/
    └── x86_64-win/
        └── AviatorKeyz.vst3   (the actual DLL)
```
JUCE's CMake build creates this structure automatically. Do not manually rename the DLL.

### GVST-002 — Tail length
`getTailLengthSeconds()` returns 2.0 to give the reverb tail time to decay after notes end. If this is too long (causing host to delay track bounce), reduce to match actual reverb max decay time in M2.

---

*Add new entries as host-specific issues are discovered during testing.*
