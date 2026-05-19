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
