# AviatorKeyz — Premium Sample-Based VST3 Instrument

A high-end, sample-based virtual instrument built with JUCE and C++20, targeting VST3 format. Inspired by the polish and musicality of Kontakt and Analog Lab — built for brass, strings, ensembles, pads, leads, and more.

---

## Requirements

| Tool | Version |
|------|---------|
| CMake | 3.22+ |
| Visual Studio | 2022 (Windows) |
| Git | Any recent |
| JUCE | Fetched automatically by CMake |

**Platform:** Windows 10/11 (64-bit). macOS support planned.

---

## Build (Windows)

```batch
# Release build
scripts\build_windows.bat

# Debug build
scripts\build_windows.bat debug

# Clean
scripts\build_windows.bat clean
```

Or manually with CMake:

```batch
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

Output: `build\AviatorKeyz_artefacts\Release\VST3\AviatorKeyz.vst3`

---

## Install for FL Studio Testing

1. Build the plugin (release or debug)
2. Copy `AviatorKeyz.vst3` to:
   ```
   C:\Program Files\Common Files\VST3\
   ```
3. Open FL Studio
4. Go to **Options → Manage plugins → Find more plugins**
5. Set scan path to `C:\Program Files\Common Files\VST3\`
6. Click **Start scan**
7. AviatorKeyz should appear under **Instruments → VST3 → AviatorKeyz**

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
│   │   └── PresetManager.* Preset load/save, category browser data
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
│   └── Presets/Factory/    Bundled factory presets (M3)
├── scripts/
│   └── build_windows.bat
└── docs/
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
M1  Sampler + MIDI            ██████████░░   85%  ← current
M2  Creative DSP              ████████░░░░   70%
M3  Presets                    ████░░░░░░░░   25%
M4  Premium UI                 ██████░░░░░░   50%
M5  FL Studio certification    ░░░░░░░░░░░░    0%
```

| # | Title | Status |
|---|-------|--------|
| M0 | Scaffold & Build | ✅ Complete (committed) |
| M1 | Core Sampler Engine | 🔧 ~85% — see [PROGRESS.md](docs/PROGRESS.md) |
| M2 | Creative Engine | ~70% wired in working tree |
| M3 | Preset System | ~25% |
| M4 | Premium UI | ~50% |
| M5 | Polish & FL Studio Cert | Not started |

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
