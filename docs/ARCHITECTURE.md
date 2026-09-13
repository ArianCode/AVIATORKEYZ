# AviatorKeyz — Architecture

**Version:** 1.0 (M0 baseline — locked)
**Date:** 2026-04-15

This document describes the module structure, layer responsibilities, threading model, and key design decisions. It is the engineering source of truth. No silent architectural changes are permitted — all major decisions must be documented here.

---

## Module Tree

```
AviatorKeyz
│
├── DSP Layer           (audio thread only)
│   ├── SamplerEngine       Polyphonic voice pool, sample-to-MIDI mapping
│   ├── ReversePlayer       Per-voice read direction (forward / backward)
│   ├── GlideEngine         Portamento pitch ramp between notes
│   ├── SmearProcessor      Transient blur / diffusion effect
│   ├── ToneShaper          Single-knob tilt/shelf EQ coloring
│   ├── ReverbTail          Space section reverb (equal-power mix over AviationReverb)
│   └── Reverb/AviationReverb  Multi-topology reverb: Plate/Hall/Room/Cloud/Hardware + Modern/Vintage color
│
├── State Layer         (message thread)
│   ├── StateSchema         Frozen param IDs + schema version constant (header only)
│   ├── PresetManager       Factory + user preset enumerate / load / save
│   └── SampleLibrary       Sample file registry, MIDI mapping metadata, user import
│
├── GUI Layer           (message thread)
│   ├── LuxuryLookAndFeel   JUCE L&F subclass — dark gold visual identity
│   ├── MainPanel           Root layout component
│   ├── HeaderBar           Logo, preset name, navigation tabs
│   ├── WaveformDisplay     Sample waveform viewer + playhead
│   ├── KnobComponent       Reusable APVTS-attached rotary knob
│   ├── PresetBrowser       Category list + preset scroll
│   └── SampleMapEditor     Sample import + MIDI range assignment (M3+)
│
├── MIDI Layer          (audio thread)
│   └── MidiHandler         Note-on/off routing, CC, sustain, all-notes-off
│
└── Host Integration
    ├── PluginProcessor     AudioProcessor root — owns APVTS, orchestrates DSP
    └── PluginEditor        AudioProcessorEditor root — owns GUI, wires APVTS
```

---

## Layer Separation Rules

These rules are non-negotiable and must never be violated:

| From | To | Allowed? | Method |
|---|---|---|---|
| DSP | DSP | Yes | Direct call |
| DSP | State | No | — |
| DSP | GUI | No | — |
| GUI | APVTS | Yes | SliderAttachment / getRawParameterValue |
| GUI | DSP | **No** | — |
| GUI | State | Yes | Via message thread only |
| State | DSP | No | — |
| Audio thread | Allocations | **No** | Fixed pools only |
| Audio thread | Locks | **No** | Lock-free only |

DSP → GUI feedback (e.g. waveform playhead position) is permitted only via `std::atomic` or lock-free ring buffer. No direct calls.

---

## Threading Model

```
Audio thread:    processBlock() → MidiHandler → SamplerEngine → DSP chain
Message thread:  GUI paint/resize, APVTS writes, PresetManager, file I/O
```

APVTS provides the bridge: parameters are written from the message thread (host automation, UI interaction) and read atomically from the audio thread via `getRawParameterValue()->load()`.

---

## State Flow

```
Host → getStateInformation()
  apvts.copyState()
  state.setProperty("stateVersion", N)
  state.createXml() → copyXmlToBinary() → MemoryBlock

Host → setStateInformation()
  getXmlFromBinary() → ValueTree::fromXml()
  check stateVersion → migrate if needed
  apvts.replaceState()
```

---

## Parameter Compatibility Contract

1. All parameter IDs live in `Source/State/StateSchema.h` — nowhere else.
2. IDs are `constexpr` string literals. They must never be renamed or removed after 1.0 release.
3. New parameters may be appended. Hosts will receive defaults for unknown parameters.
4. When binary state format changes incompatibly, `STATE_SCHEMA_VERSION` is bumped and migration logic is added in `setStateInformation()`.
5. Preset files embed `stateVersion`. `PresetManager::loadPreset()` runs migration before calling `apvts.replaceState()`.

---

## Voice Architecture (M1)

- Fixed pool of `kMaxVoices = 64` polyphonic voices.
- Voice stealing: steal the quietest currently-playing voice.
- No heap allocations inside `processBlock`. All voice state is pre-allocated at `prepareToPlay`.
- Sample buffers: loaded on message thread via double-buffer swap. Audio thread reads from a consistent, non-mutating snapshot.
- Pitch ratio: `pow(2.0f, (midiNote - rootNote) / 12.0f)` — linear resampling in M1, interpolated in M2.

---

## DSP Chain Order (M2)

Per-voice: SamplerEngine (with ReversePlayer read direction + GlideEngine pitch)
Post-mix: ToneShaper → SmearProcessor → ReverbTail → StereoWidth → OutputGain

---

## Key Design Decisions

**APVTS over manual parameter management**
APVTS provides thread-safe atomics, XML serialization, and host automation at negligible cost for 9 parameters. Manual management would require rebuilding all of this.

**Fixed voice pool**
Dynamic voice allocation on the audio thread causes priority inversion and unpredictable latency. 64 fixed voices = known worst-case memory, O(1) allocation.

**GlideEngine separate from SamplerEngine**
Glide is a pitch-source concern. Separating it lets the ramp apply correctly regardless of whether Reverse is active, and keeps both classes unit-testable in isolation.

**StateSchema as a pure header**
No instances, no constructors, no linking. Every module that needs a parameter ID `#include`s this header. There is no risk of divergence between the "parameter system" and the "preset system" — they both read the same constants.

**LuxuryLookAndFeel as a JUCE L&F subclass**
Centralizing all visual overrides in one class means changing the gold accent color or knob style requires editing exactly one file, not hunting through component paint() methods.
