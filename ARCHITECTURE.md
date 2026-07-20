# AviatorKeyz — Architecture Document

**Version:** 0.1 (M0 Scaffold)
**Last updated:** 2026-04-15

---

## Overview

AviatorKeyz is a polyphonic, sample-based VST3 instrument. Its architecture is divided into four layers with strict separation of concerns:

1. **DSP Layer** — audio-thread-only signal processing
2. **State Layer** — serialization, preset I/O, parameter management
3. **GUI Layer** — message-thread-only rendering and interaction
4. **MIDI Layer** — event routing from host to DSP

Each layer communicates with adjacent layers through JUCE's `AudioProcessorValueTreeState` (APVTS) and controlled handoff points — never through direct coupling.

---

## Layer: DSP

**Rule:** No allocations, no GUI calls, no locks inside `processBlock` or any method called from it.

### SamplerEngine
Manages a fixed pool of `kMaxVoices = 64` polyphonic voices. Each voice holds a reference to a sample buffer and a read position. On note-on, an available voice is assigned; on note-off (and after release envelope), it is returned to the pool. Voice stealing uses "steal quietest" strategy.

Sample buffers are loaded and swapped via a double-buffer idiom on the message thread. The audio thread always reads from a consistent, non-mutating snapshot.

### ToneShaper
A tilt EQ implemented as two cascaded IIR shelving filters. Low-shelf and high-shelf coefficients are parameterized by the `tone` value. Coefficients are recomputed every ~64 samples when the smoothed tone value changes, rather than per-sample, to avoid excessive CPU use.

### SmearProcessor
A transient blur effect. Implementation uses a short convolution with a variable-length exponential decay kernel, wet/dry blended by the `smear` value. Alternative approach (multi-tap diffusion delay) may be substituted based on M2 CPU profiling.

### GlideEngine
Tracks the active pitch (in semitones) with a linear ramp between note pitches. The ramp length is set by `glide_time` in milliseconds. The engine ticks once per sample and provides `getCurrentPitchSemitones()` to SamplerEngine for pitch ratio calculation.

### ReversePlayer
Provides a read-increment sign utility. When `reverse = true`, the per-voice read increment is negated, and the voice starts at the end of its sample buffer. This keeps the note's MIDI onset time locked to its original position — only the buffer read direction changes.

---

## Layer: State

### APVTS (AudioProcessorValueTreeState)
All automatable parameters are declared in `createParameterLayout()` in `PluginProcessor.cpp`. This is the single source of truth for the plugin's automation-facing interface.

**Compatibility contract:** Parameter IDs in `StateSchema.h` are frozen once shipped. Hosts embed these IDs in saved projects. Renaming or removing any ID will break those projects.

### PresetManager
Handles factory and user preset enumeration, loading, and saving. Factory presets live in `Resources/Presets/Factory/` and are bundled as binary resources via `juce_add_binary_data`. User presets live in `~/Documents/AviatorKeyz/Presets/`.

Preset files are XML documents containing the APVTS `ValueTree` state plus metadata (`name`, `category`, `author`, `schemaVersion`). When `STATE_SCHEMA_VERSION` is bumped, `PresetManager::loadPreset()` runs migration before calling `apvts.replaceState()`.

### StateSchema
A pure header — no instances, only `constexpr` string literals and integer constants. Every other module includes this header; nothing replaces it.

---

## Layer: GUI

**Rule:** GUI components must never call DSP methods. All state access goes through APVTS attachments or `getRawParameterValue()`.

### LuxuryLookAndFeel
A `LookAndFeel_V4` subclass providing the visual identity:
- Dark near-black background
- Gold primary accent (`#C8922A`)
- Warm off-white text
- Custom rotary knob draw (thin arc track, gold pointer, glow on hover)

### Component tree (Aviation MAIN interface)

The MAIN view lives in `Source/GUI/Aviation/` on a fixed 1647x955 design
canvas; the editor scales it as one unit (fixed aspect ratio, default
1100x638). Every element is centered/mirrored on the global center axis.

```
AviatorKeyzEditor
├── AviationMainView            (1647x955 design space, scaled via transform)
│   ├── CockpitBackground       (full-width cockpit photo, center-axis pan,
│   │                            drop-in slot for cockpit_sunset_* asset)
│   ├── TopHeader               (brand | MAIN/PERFORMANCE | gear, about, SAVE)
│   ├── PresetHeader            (FACTORY PRESETS | prev/name/next | heart)
│   ├── CategoryTabs            (10 categories, version stamp)
│   ├── SourceDropdown          (top-right preset selector -> luggage menu)
│   ├── CenterDashboard         (RPM/KEY/TUNE, radars, blueprint, GLOBALS
│   │                            output gain, LIMITER, LOFI..HUMANIZE cells)
│   ├── VelocityPanel / LayerMixPanel      (left cockpit glass displays)
│   ├── FilterPanel / EnvelopePanel        (right cockpit glass displays)
│   ├── MacroDeck               (8 MacroKnobs mirrored around brand block)
│   ├── StatusBar               (ACTIVE, rate/bits/BPM, A/B, version)
│   └── MenuLookAndFeel         (overhead-luggage popup styling)
└── AdvancedPanel               (PERFORMANCE view, shown below the header)
```

Legacy `MainPanel`/`CockpitCrossworldPanel` components remain compiled but
are no longer in the visible hierarchy.

---

## Layer: MIDI

MidiHandler is called at the top of `processBlock`. It iterates the MidiBuffer and routes events to SamplerEngine and GlideEngine. No state is written from the audio thread — sustain pedal state is held as a simple `bool` on MidiHandler, which is only accessed from the audio thread.

---

## Threading Model

| Thread | Allowed operations |
|--------|--------------------|
| Audio thread | `processBlock`, DSP module `process()`, atomic APVTS reads |
| Message thread | GUI paint/resize, APVTS writes, PresetManager, file I/O |
| Any | Read `constexpr` values from StateSchema |

DSP modules communicate back to the GUI (e.g. waveform playhead position) via lock-free ring buffers or `std::atomic` values. Direct calls from DSP → GUI are prohibited.

---

## State Serialization Flow

```
Host calls getStateInformation()
  → apvts.copyState()           // deep copy of ValueTree
  → state.setProperty("stateVersion", N)
  → state.createXml()
  → copyXmlToBinary()           // into MemoryBlock

Host calls setStateInformation()
  → getXmlFromBinary()
  → ValueTree::fromXml()
  → check stateVersion, run migrations if needed
  → apvts.replaceState()        // updates all parameters atomically
```

---

## Preset Format (v1)

```xml
<AviatorKeyzState stateVersion="1">
  <PARAM id="input_gain"    value="0.0"/>
  <PARAM id="output_gain"   value="0.0"/>
  <PARAM id="reverse"       value="0"/>
  <PARAM id="glide_time"    value="0.0"/>
  <PARAM id="smear"         value="0.0"/>
  <PARAM id="tone"          value="0.0"/>
  <PARAM id="reverb_amount" value="0.0"/>
  <PARAM id="reverb_size"   value="0.5"/>
  <PARAM id="stereo_width"  value="1.0"/>
  <PresetMeta category="Leads" name="Night Flight" author="AviatorKeyz"/>
</AviatorKeyzState>
```

---

## Key Design Decisions

**Why APVTS instead of manual parameter management?**
APVTS provides host automation, undo/redo, XML state serialization, and thread-safe atomic access out of the box. The cost is mild — a small amount of overhead per parameter. At our scale (9 params), this is negligible.

**Why fix schema version at construction?**
Hosts replay automation by parameter ID. If an ID changes between plugin versions, the host's stored value is silently ignored. StateSchema.h makes this impossible to change accidentally.

**Why fixed-size voice pool instead of dynamic allocation?**
Dynamic allocation on the audio thread causes priority inversion and unpredictable latency. A fixed pool of 64 voices means worst-case memory is known at compile time, and voice allocation is O(1).

**Why separate GlideEngine from SamplerEngine?**
Glide is a pitch-source concern, not a sample-playback concern. Separating them allows the glide to be applied correctly even when Reverse is active, and keeps each class testable in isolation.
