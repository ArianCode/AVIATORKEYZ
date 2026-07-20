# AviatorKeyz — Premium Sample-Based VST3 Instrument

A high-end, sample-based virtual instrument built with JUCE and C++20, targeting VST3 format. Inspired by the polish and musicality of Kontakt and Analog Lab — built for brass, strings, ensembles, pads, leads, and more.

---

## Local Development Workflow (Windows + macOS)

### 1) Required software

#### Windows (Visual Studio workflow)

- Windows 10/11 x64
- Visual Studio 2022 with `Desktop development with C++`
  - MSVC v143
  - Windows 10/11 SDK
  - MSBuild + CMake tools
- CMake 3.22+
- Git

Optional but useful:

- REAPER (quick VST3 host)
- JUCE AudioPluginHost (`extras/AudioPluginHost`) for quick plugin validation

#### macOS (Xcode workflow)

- macOS 13+
- Xcode 15+ (or compatible with your installed SDK)
- Xcode Command Line Tools
- CMake 3.22+
- Git

Optional but useful:

- REAPER/Ableton/Logic Pro for host testing
- JUCE AudioPluginHost for quick plugin validation
- Ninja (`brew install ninja`) if you prefer non-Xcode command-line builds

### 2) Generate project files (if needed)

#### Windows (Visual Studio 2022 solution)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
```

Open `build\AviatorKeyz.sln` in Visual Studio.

#### macOS (Xcode project)

```bash
cmake -S . -B build-xcode -G Xcode
```

Open `build-xcode/AviatorKeyz.xcodeproj` in Xcode.

### 3) Build commands

#### Windows quick build (scripted)

```powershell
# Release
.\scripts\build_windows.bat

# Debug
.\scripts\build_windows.bat debug

# Clean
.\scripts\build_windows.bat clean
```

#### Windows manual CMake build

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

#### macOS quick build (scripted)

```bash
chmod +x scripts/build_macos.sh

# Release
./scripts/build_macos.sh

# Debug
./scripts/build_macos.sh debug

# Release + install VST3
./scripts/build_macos.sh install

# Clean
./scripts/build_macos.sh clean

# RelWithDebInfo
./scripts/build_macos.sh relwithdebinfo

# Sanitizer unit tests only (tests-only tree — never install plugin bundles from here)
./scripts/build_macos.sh sanitizer-tests
```

Equivalent manual configure:

```bash
cmake -S . -B build-asan -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DAVIATORKEYZ_BUILD_PLUGIN=OFF \
  -DAVIATORKEYZ_BUILD_TESTS=ON \
  -DAVIATORKEYZ_ENABLE_SANITIZERS=ON
cmake --build build-asan --target AviatorKeyzSanitizerTests
```

Production VST3/AU/Standalone builds never use AddressSanitizer, UndefinedBehaviorSanitizer, or ThreadSanitizer. Debug builds remain host-loadable. Install and verify:

```bash
./scripts/build_macos.sh install
./scripts/verify_plugin_binary.sh \
  ~/Library/Audio/Plug-Ins/VST3/AviatorKeyz.vst3/Contents/MacOS/AviatorKeyz
```

FL Studio checklist: [docs/FL_STUDIO_VALIDATION.md](docs/FL_STUDIO_VALIDATION.md)  
Primary test host: **FL Studio 2025 25.2.3.4889** on macOS 15.7.4 — [docs/HOST_TEST_CONFIG.md](docs/HOST_TEST_CONFIG.md)

#### macOS manual CMake build

```bash
# Xcode generator
cmake -S . -B build-xcode -G Xcode
cmake --build build-xcode --config Release

# or Ninja
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### 4) Build outputs

Typical output locations:

- VST3: `build/AviatorKeyz_artefacts/Release/VST3/AviatorKeyz.vst3`
- Standalone:
  - macOS: `build/AviatorKeyz_artefacts/Release/Standalone/AviatorKeyz.app`
  - Windows: `build/AviatorKeyz_artefacts/Release/Standalone/AviatorKeyz.exe`
- AU (macOS): `build/AviatorKeyz_artefacts/Release/AU/AviatorKeyz.component`

### 5) Test in DAW or plugin host

#### Windows (VST3)

1. Build Release.
2. Copy:
   - `build\AviatorKeyz_artefacts\Release\VST3\AviatorKeyz.vst3`
   - to `C:\Program Files\Common Files\VST3\`
3. Rescan plugins in your DAW.
4. Load AviatorKeyz as an instrument plugin.

For quick host testing, use REAPER or JUCE AudioPluginHost and scan the same VST3 folder.

#### macOS (AU + VST3)

1. Build Release.
2. Copy/install artifacts:
   - VST3 to `~/Library/Audio/Plug-Ins/VST3/`
   - AU component to `~/Library/Audio/Plug-Ins/Components/`
3. Rescan/restart host:
   - Logic uses AU
   - Ableton/REAPER can use VST3
4. Load AviatorKeyz as an instrument.

### 6) Fast UI preview without a DAW

Use the Standalone target for rapid GUI iteration:

#### Windows

```powershell
.\scripts\run_standalone_windows.bat
```

#### macOS

```bash
chmod +x scripts/run_standalone_macos.sh
./scripts/run_standalone_macos.sh
```

Recommended UI loop:

1. Build Debug or Release standalone
2. Launch standalone
3. Iterate on `Source/GUI/*`
4. Rebuild + relaunch

Factory sounds ship inside the plugin (embedded WAV + presets), not as a separate host sound-bank file.

Pre-release checks: `./scripts/verify_release.sh`

Full deliverables checklist: [docs/DELIVERABLES.md](docs/DELIVERABLES.md)

---

## Project Structure

```
AviatorKeyz/
├── CMakeLists.txt          Build definition
├── Source/
│   ├── PluginProcessor.*   AudioProcessor root, APVTS, state I/O
│   ├── PluginEditor.*      Editor root, resize, layout
│   ├── State/
│   │   ├── StateSchema.h   Parameter IDs + schema version (NEVER change IDs)
│   │   ├── FactoryResources.* Embedded preset/WAV lookup
│   │   ├── PresetManager.* Preset load/save, category browser data
│   │   └── SampleLibrary.*   User WAV/AIFF import (M3+)
│   ├── DSP/
│   │   ├── SamplerEngine.* Polyphonic sample playback (M1)
│   │   ├── ToneShaper.*    Single-knob tilt EQ (M2)
│   │   ├── SmearProcessor.*Transient blur control (M2)
│   │   ├── GlideEngine.*   Portamento pitch tracking (M2)
│   │   └── ReversePlayer.* Reversed sample playback (M2)
│   ├── GUI/
│   │   ├── Aviation/       MAIN cockpit interface (1647x955 design canvas):
│   │   │                   TopHeader, PresetHeader, CategoryTabs, preset
│   │   │                   dropdown, CenterDashboard, side panels, MacroDeck,
│   │   │                   StatusBar, luggage-style menus
│   │   ├── Advanced/       PERFORMANCE page (LFOs, FX, mod matrix, chop)
│   │   ├── LuxuryLookAndFeel.* Dark gold visual identity
│   │   └── Cockpit/ + legacy components (out of the visible hierarchy)
│   └── MIDI/
│       └── MidiHandler.*   Note routing, CC, sustain (M1)
├── Resources/
│   ├── Fonts/
│   ├── Images/
│   ├── Factory/            Embedded factory WAV (per category)
│   └── Presets/Factory/    50 bundled factory presets (M3)
├── scripts/
│   ├── build_macos.sh      Primary build script
│   ├── build_windows.bat   Secondary (Windows)
│   ├── run_standalone_macos.sh
│   ├── run_standalone_windows.bat
│   ├── generate_factory_assets.py
│   ├── validate_factory_presets.py
│   └── verify_release.sh
└── docs/
    ├── DELIVERABLES.md
    ├── CONTENT_PIPELINE.md
    ├── CODE_SIGNING.md
    ├── ARCHITECTURE.md
    ├── TESTPLAN.md
    ├── CHANGELOG.md
    ├── TODO.md
    └── known-host-issues.md
```

---

## Milestones

Visual tracker (updated 2026-05-18): **[docs/PROGRESS.md](docs/PROGRESS.md)**

```
M0  Scaffold & build          ████████████  100%
M1  Sampler + MIDI            ██████████░░   90%
M2  Creative DSP              █████████░░░   85%
M3  Presets                    █████████░░░   80%
M4  Premium UI                 ███████░░░░░   60%
M5  Host certification         ██░░░░░░░░░░   15%
```

| # | Title | Status |
|---|-------|--------|
| M0 | Scaffold & Build | ✅ Complete (committed) |
| M1 | Core Sampler Engine | ~90% — host exit tests open |
| M2 | Creative Engine | ~85% — automation validation open |
| M3 | Preset System | ~110 factory presets embedded |
| M4 | Premium UI | Photo-anchored cockpit + Advanced view |
| M5 | Polish & host cert | Automated smoke + manual DAW QA — see [M5_CERTIFICATION.md](docs/M5_CERTIFICATION.md) |

---

## Parameter IDs

These are fixed after 1.0 release. See `Source/State/StateSchema.h`.

| ID | Name | Range | Default |
|----|------|-------|---------|
| `input_gain` | Input Gain | -24→+12 dB | 0 |
| `output_gain` | Output Gain | -24→+12 dB | 0 |
| `reverse` | Reverse | bool | false |
| `glide_time` | Glide | 0→500 ms | 0 |
| `smear` | Smear | 0→1 | 0 |
| `tone` | Tone | -1→+1 | 0 |
| `reverb_amount` | Reverb | 0→1 | 0 |
| `reverb_size` | Reverb Size | 0→1 | 0.5 |
| `stereo_width` | Width | 0→2 | 1.0 |

---

## Contributing / Development Notes

- Never rename or remove a parameter ID after it has shipped.
- All DSP work must be allocation-free and lock-free on the audio thread.
- UI components must never call DSP methods directly — only APVTS.
- When `STATE_SCHEMA_VERSION` bumps, add migration logic in `setStateInformation()`.
- See `ARCHITECTURE.md` for full module design rationale.
