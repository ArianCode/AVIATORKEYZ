# AviatorKeyz — Premium Sample-Based VST3 Instrument

A high-end, sample-based virtual instrument built with JUCE and C++20, targeting VST3 format. Inspired by the polish and musicality of Kontakt and Analog Lab — built for brass, strings, ensembles, pads, leads, and more.

---

## Requirements

| Tool | Version |
|------|---------|
| CMake | 3.22+ |
| Xcode Command Line Tools | macOS 13+ (full Xcode for GUI debugging) |
| Ninja | Recommended (`brew install ninja`) |
| Git | Any recent |
| JUCE | Fetched automatically by CMake |

**Primary platform:** macOS 13+ (Apple Silicon and Intel).  
**Secondary platform:** Windows 10/11 x64 (see `scripts/build_windows.bat`).

---

## Build (macOS)

```bash
chmod +x scripts/build_macos.sh

# Release build
./scripts/build_macos.sh

# Debug build
./scripts/build_macos.sh debug

# Release + install VST3 to ~/Library/Audio/Plug-Ins/VST3/
./scripts/build_macos.sh install

# Clean
./scripts/build_macos.sh clean
```

Or manually with CMake (Ninja):

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

**Outputs:** the build script prints exact paths. On macOS they are typically:

- VST3: `build/AviatorKeyz_artefacts/Release/VST3/AviatorKeyz.vst3`
- Standalone: `build/AviatorKeyz_artefacts/Release/Standalone/AviatorKeyz.app`

---

## Install for DAW testing (macOS)

1. Build the plugin (release or debug)
2. Copy the bundle:
   ```bash
   cp -R build/AviatorKeyz_artefacts/Release/VST3/AviatorKeyz.vst3 \
     ~/Library/Audio/Plug-Ins/VST3/
   ```
   Or use `./scripts/build_macos.sh install`
3. Rescan plugins in your DAW (Logic, Ableton, Reaper, FL Studio for Mac, etc.)
4. AviatorKeyz should appear as a **VST3 instrument** (not an effect)

**Fast dev loop:** run the Standalone app — no DAW required:

```bash
open build/AviatorKeyz_artefacts/Release/Standalone/AviatorKeyz.app
```

Factory sounds ship inside the plugin (embedded WAV + presets), not as a separate host sound-bank file.

Pre-release checks: `./scripts/verify_release.sh`

Full deliverables checklist: [docs/DELIVERABLES.md](docs/DELIVERABLES.md)

---

## Build (Windows) — secondary

```batch
scripts\build_windows.bat
```

Output: `build\AviatorKeyz_artefacts\Release\VST3\AviatorKeyz.vst3`  
Install to: `C:\Program Files\Common Files\VST3\`

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
│   │   ├── LuxuryLookAndFeel.* Dark gold visual identity (M4)
│   │   ├── MainPanel.*     Root layout component (M4)
│   │   ├── HeaderBar.*     Logo + preset name + tabs (M4)
│   │   ├── KnobComponent.* Reusable premium knob (M4)
│   │   ├── WaveformDisplay.*Live sample viewer (M4)
│   │   └── PresetBrowser.* Category + preset list (M3)
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
M1  Sampler + MIDI            ████████████  100%
M2  Creative DSP              ████████████  100%
M3  Presets                    ████████████  100%
M4  Premium UI                 ██████░░░░░░   50%
M5  Host certification         ██░░░░░░░░░░   15%
```

| # | Title | Status |
|---|-------|--------|
| M0 | Scaffold & Build | ✅ Complete (committed) |
| M1 | Core Sampler Engine | ✅ Complete |
| M2 | Creative Engine | ✅ Complete |
| M3 | Preset System | ✅ 50 factory presets embedded |
| M4 | Premium UI | 🔧 In progress |
| M5 | Polish & host cert | Manual QA on macOS + signing — see [CODE_SIGNING.md](docs/CODE_SIGNING.md) |

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
