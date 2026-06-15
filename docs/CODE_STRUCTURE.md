# AviatorKeyz — Code Structure

**Purpose:** High-level map of the repository — where code lives, how modules connect, and what each layer owns. For design rules and threading constraints, see [ARCHITECTURE.md](ARCHITECTURE.md).

**Last updated:** 2026-06-14

---

## Repository Overview

```
AviatorKeyz/
├── CMakeLists.txt          Build entry — JUCE fetch, plugin target, embedded assets
├── Source/                 All C++ plugin code
├── Resources/              Factory WAVs, preset XML, cockpit UI images (embedded at build)
├── ContentImport/          Staging area for licensed sample bank import (not embedded directly)
├── scripts/                Build, content pipeline, and release tooling
├── tests/                  C++ unit tests + Python validation tests
├── docs/                   Engineering and product documentation
├── licenses/               Legal, clearance, and third-party notices
└── build*/                 Local build output (gitignored)
```

**Build targets:** VST3, AU (macOS), and Standalone — defined in `CMakeLists.txt` via `juce_add_plugin`.

**Framework:** [JUCE 8.0.9](https://github.com/juce-framework/JUCE) (fetched automatically by CMake).

---

## Entry Points

| File | Role |
|------|------|
| `Source/PluginProcessor.h/.cpp` | Root `AudioProcessor` — owns APVTS, DSP engines, preset/sample state, `processBlock` |
| `Source/PluginEditor.h/.cpp` | Root `AudioProcessorEditor` — switches between Cockpit and Advanced views |

The processor is the single orchestrator. The editor never calls DSP directly; it reads and writes parameters through APVTS attachments.

---

## Source Tree

```
Source/
├── PluginProcessor.*       Host integration + audio graph
├── PluginEditor.*            Top-level UI shell
│
├── State/                    Message-thread state, presets, parameters
├── DSP/                      Audio-thread signal processing
├── MIDI/                     MIDI event routing (audio thread)
├── GUI/                      Message-thread UI
│   ├── Cockpit/              Photo-anchored main performance view
│   ├── Advanced/             Deep engine / mod / FX editor
│   └── _legacy/              Superseded UI components (not in CMake target)
└── Debug/                    Debug logging helpers
```

---

## Layer: State (`Source/State/`)

Handles serialization, preset I/O, parameter definitions, and sample metadata. Runs on the message thread except for published snapshots read by the audio thread.

| File | Responsibility |
|------|----------------|
| `StateSchema.h` | **Single source of truth** for parameter IDs, schema version, preset keys, sample IDs |
| `ParameterLayout.h/.cpp` | Builds the APVTS parameter layout (ranges, defaults, choices) |
| `ApvtsStateHelpers.h/.cpp` | State save/load helpers, migration hooks |
| `PresetManager.h/.cpp` | Factory + user preset enumerate / load / save |
| `SampleLibrary.h/.cpp` | Sample buffer registry; double-buffer publish for thread-safe audio reads |
| `FactoryResources.h/.cpp` | Lookup for embedded WAV/preset binary data (`AviatorKeyzBinary` namespace) |

**Key contract:** Parameter ID strings are defined only in `StateSchema.h`. All other modules reference `AviatorKeyz::ParamID::*`.

---

## Layer: DSP (`Source/DSP/`)

Audio-thread-only processing. No allocations, GUI calls, or locks inside `processBlock`.

### Sound sources

| Module | Role |
|--------|------|
| `SamplerEngine` | Polyphonic sample playback (64-voice pool, voice stealing) |
| `SynthEngine` | Wavetable/oscillator synthesis layer |
| `ReversePlayer` | Per-voice read direction (forward / backward) |
| `GlideEngine` | Portamento pitch ramp (used by sampler voices) |
| `PitchProbe` | Pitch detection utility for content pipeline alignment |

### Tone shaping & effects

| Module | Role |
|--------|------|
| `FilterProcessor` | Multi-mode filter with envelope modulation |
| `TextureEngine` | Granular / spectral texture processing |
| `ToneShaper` | Single-knob tilt EQ |
| `SmearProcessor` | Transient blur / diffusion |
| `ReverbTail` | Plate-style reverb |
| `FxChain` | Delay, chorus, lo-fi, distortion |
| `OutputLimiter` | Final peak limiting |

### Modulation

| Module | Role |
|--------|------|
| `LfoEngine` | Three LFOs with host sync |
| `ModMatrix` | Routes LFOs and sources to parameter offsets |
| `PerformanceMacroEngine` | Four performance macros that fan out to multiple targets |

---

## Layer: MIDI (`Source/MIDI/`)

| Module | Role |
|--------|------|
| `MidiHandler` | Parses `MidiBuffer` at the top of `processBlock`; routes note-on/off, sustain, CC to `SamplerEngine` and `SynthEngine` |

---

## Layer: GUI (`Source/GUI/`)

Message-thread-only. Components attach to APVTS via `SliderAttachment`, `ButtonAttachment`, etc.

### Shared components

| File | Role |
|------|------|
| `MainPanel` | Cockpit view root — hosts `CockpitCrossworldPanel` |
| `AdvancedPanel` | Advanced view root — hosts `AdvancedPageContent` |
| `ViewModeTabBar` | Cockpit ↔ Advanced tab switcher |
| `LuxuryLookAndFeel` | Dark/gold visual identity (JUCE L&F subclass) |
| `AviatorTokens.h` / `DesignTokens.h` | Layout constants, scaling, colors |
| `PrecisionKnob` | Reusable APVTS-attached rotary control |
| `ReverseToggle` | Reverse playback toggle |
| `PresetBrowser` | Category + preset list (popup / overlay) |
| `FooterBar` | Bottom status / branding bar |

### Cockpit view (`Source/GUI/Cockpit/`)

Photo-anchored performance UI — knobs positioned over a cockpit photograph.

```
CockpitCrossworldPanel
├── CockpitPhotoBackground       Full-bleed cockpit image
├── CockpitTopPresetBar           Category tabs
├── CockpitPresetControlBar       Prev / next / save / favourite
├── PresetCenterNavigator         Horizon-style preset name display
├── AdvancedPresetSidebar         Preset list sidebar
├── PresetSearchOverlay           Search overlay
├── InstrumentPanelBar            Aviation-style gauge row
├── PhotoAnchoredKnob (×N)        Controls placed at photo anchor points
└── FooterBar
```

Supporting files: `CockpitZones`, `CockpitLayout`, `AviationGauge`, `PresetDisplayUtils`, `PresetHorizonReadout`, `PresetNavButton`, `PresetIconButtons`, `SearchIconButton`.

Layout anchor data lives in `CockpitZones.h/.cpp` and `CockpitLayout.h`.

### Advanced view (`Source/GUI/Advanced/`)

Deep parameter editor — oscillators, filter, envelopes, LFOs, mod matrix, texture engine, FX routing.

```
AdvancedPanel
└── AdvancedPageContent
    ├── SynthPanelComponent         Oscillators, filter, envelopes
    ├── SourcePanelComponent        Source blend / voice settings
    ├── TextureSectionComponent     Texture engine controls + visualizer
    ├── LfoPanelComponent           Three LFO strips
    ├── ModMatrixComponent          Mod routing grid
    ├── PerformanceMacroStrip       Four macro knobs
    ├── FxAdvancedPanel             FX enable / routing
    └── Visualizers: OscWaveformDisplay, FilterCurveGraph, LfoWaveformDisplay
```

Shared widgets: `AdvancedWidgets`, `BracketValueBox`, `ModAmountSlider`, `ModRoutingHub`, `ModAssignCallout`, `AdvancedParameterLayout`, `AdvancedPresetSidebar`.

### Legacy (`Source/GUI/_legacy/`)

Earlier UI prototypes (`HeaderBar`, `WaveformDisplay`, `KnobComponent`, `PluginShell`, etc.). Not compiled into the plugin target — kept for reference during the Cockpit migration.

---

## UI Component Tree

```
AviatorKeyzEditor
├── ViewModeTabBar
├── MainPanel                          (visible in Cockpit mode)
│   └── CockpitCrossworldPanel
│       └── [cockpit sub-components — see above]
└── AdvancedPanel                      (visible in Advanced mode)
    └── AdvancedPageContent
        └── [advanced sub-components — see above]
```

---

## Audio Signal Flow

`AviatorKeyzProcessor::processBlock` runs this chain each buffer:

```
MIDI in
  └─► MidiHandler ──► SamplerEngine ──┐
                      SynthEngine  ──┤
                                       ├─► source blend mix
                                       │
                                       ▼
                              input gain (smoothed + mod)
                                       │
                                       ▼
                              FilterProcessor
                                       │
                                       ▼
                              TextureEngine
                                       │
                                       ▼
                              ToneShaper → SmearProcessor → ReverbTail
                                       │
                                       ▼
                              FxChain (delay / chorus / lo-fi / dist)
                                       │
                                       ▼
                              stereo width + pan
                                       │
                                       ▼
                              OutputLimiter → output gain
```

Modulation is computed before the chain:

1. `LfoEngine::advance`
2. `ModMatrix::updateFromApvts` — produces per-parameter offsets
3. `PerformanceMacroEngine::compute` — produces macro offsets
4. Offsets are summed into parameter reads throughout the chain

---

## Parameter & State Flow

```
Host automation / UI
        │
        ▼
   APVTS (AudioProcessorValueTreeState)
        │
        ├──► processBlock reads via getRawParameterValue()->load()
        │
        └──► getStateInformation / setStateInformation
                    │
                    ├── PresetManager (XML preset files)
                    └── Host project recall (binary XML in MemoryBlock)
```

Preset load triggers `PresetManager::onPresetLoaded` → `loadFactorySample()` → `SampleLibrary` double-buffer publish → `SamplerEngine::setSampleSnapshot`.

---

## Resources & Content Pipeline

```
Resources/
├── Factory/                  Embedded WAV files (one per factory preset)
├── Presets/Factory/          Embedded preset XML (category subfolders)
└── UI/Cockpit/               Cockpit background images
```

At build time, CMake globs these into the `AviatorKeyzData` binary target (`juce_add_binary_data`), generating `BinaryData.h` in the `AviatorKeyzBinary` namespace.

`ContentImport/` holds raw licensed samples before normalization and import via `scripts/import_factory_bank.py`. See [CONTENT_PIPELINE.md](CONTENT_PIPELINE.md) for the full workflow.

---

## Scripts (`scripts/`)

| Script | Purpose |
|--------|---------|
| `build_macos.sh` / `build_windows.bat` | Platform build helpers |
| `run_standalone_macos.sh` | Launch Standalone for quick testing |
| `generate_factory_assets.py` | Generate placeholder factory WAV + preset XML |
| `import_factory_bank.py` | Import licensed sample bank into `Resources/` |
| `validate_factory_presets.py` | Validate preset XML against schema |
| `pitch_align.py` / `normalize_root_notes.py` | Content pitch alignment |
| `verify_release.sh` / `notarize_macos.sh` | Release verification and macOS notarization |

---

## Tests (`tests/`)

### C++ unit tests (`AviatorKeyzTests`)

Built separately from the plugin (`cmake --build build --target AviatorKeyzTests`). Covers:

- `SamplerEngine`, `SampleLibrary`, `TextureEngine`, `PerformanceMacroEngine`
- `StateSchema` constants, APVTS ↔ schema parity
- `PresetManager`, pitch alignment

### Python validation tests

Run via `python3 -m pytest tests/`:

- `test_docs_structure.py` — doc presence, README links, build config
- `test_presets.py` — factory preset validation
- `test_state_schema.py` — schema consistency
- `test_factory_wavs.py` / `test_factory_pitch_alignment.py` — content integrity

---

## Build System Notes

| Concern | Location |
|---------|----------|
| Plugin identity (codes, formats) | `CMakeLists.txt` → `juce_add_plugin` |
| Source file registration | `CMakeLists.txt` → `target_sources` |
| Embedded assets | `CMakeLists.txt` → `juce_add_binary_data(AviatorKeyzData …)` |
| Include path | `Source/` (flat include root) |
| Sanitizer builds | Debug + Clang → ASan/UBSan flags in `CMakeLists.txt` |

New source files must be added to `target_sources` in `CMakeLists.txt` to be compiled.

---

## Related Documentation

| Document | Contents |
|----------|----------|
| [ARCHITECTURE.md](ARCHITECTURE.md) | Layer rules, threading model, design decisions |
| [PARAMETERS.md](PARAMETERS.md) | Full parameter reference |
| [SAMPLER_ENGINE.md](SAMPLER_ENGINE.md) | Voice pool and sample playback details |
| [CONTENT_PIPELINE.md](CONTENT_PIPELINE.md) | Factory asset import workflow |
| [UI_COCKPIT_CONTROL_MAP.md](UI_COCKPIT_CONTROL_MAP.md) | Cockpit knob → parameter mapping |
| [PRODUCT_SPEC.md](PRODUCT_SPEC.md) | Product requirements |
| [TESTPLAN.md](../TESTPLAN.md) | Manual and automated test plan |
