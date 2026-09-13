# AviatorKeyz — Product Specification

**Version:** 1.0 (locked for M0)
**Date:** 2026-04-15
**Status:** ACTIVE — this document is the source of truth for product scope

---

## What It Is

AviatorKeyz is a **premium polyphonic sample-based VST3 instrument**. It targets producers, composers, and musicians who want the sound quality and creative feel of top-tier instruments (Kontakt, Analog Lab) in a format that is fast, inspiring, and visually premium — without the complexity of a deep technical sampler.

It is NOT a general-purpose sampler. It is a curated instrument with a strong editorial voice.

---

## Format

| Property | Value |
|---|---|
| Plugin format | VST3 |
| Plugin type | Instrument (synth) |
| MIDI input | Yes |
| Audio output | Stereo |
| Primary DAW targets | Logic Pro, Ableton Live, Reaper (macOS) |
| Primary platform | macOS 13+ (Apple Silicon + Intel) |
| Secondary platform | Windows 10/11 x64 |
| Secondary DAW target | FL Studio (Windows / macOS) |

---

## Core Product Goals

1. **Premium preset library** organized in an inspiring browser
2. **User sample import** — load compatible audio files, map to MIDI keys, play inside the instrument
3. **Luxury UI** — dark gold aviation aesthetic, minimal, polished, memorable
4. **Signature sound engine** — four controls that make every preset feel more alive

---

## Preset Categories

The browser must present presets in exactly these 10 categories, in this order:

1. Leads
2. Brass
3. Ensembles
4. Strings
5. Pads
6. Phrases
7. Synths
8. Arps
9. Vocals
10. Bells

---

## Signature Sound Engine Controls

These four controls are the heart of the product. They appear consistently across all presets and define the character of the instrument.

### Reverse
Reverses the playback direction of each triggered note. The note's MIDI timing and rhythmic position in the host timeline are preserved — only the sample reads backward. This is a per-voice flag, not a global buffer flip.

### Glide
Adds smooth portamento pitch transition between notes. Pitch slides over a controllable time (0–500 ms) instead of jumping instantly. Applied to the pitch source; independent of Reverse.

### Smear
A blur control. As it increases, sharp transients and note edges soften into a smoother, more stretched, atmospheric texture. Makes sounds feel more washed and less defined.

### Tone
Shapes overall sound color with a single control. Negative values: darker, warmer. Positive values: brighter, cleaner. Implemented as a tilt/shelf EQ. No detailed EQ knowledge required from the user.

---

## What This Is NOT

- Not a full sampler with keygroup editing, round-robin, scripting, or effects routing
- Not a sound design tool with modulation matrix, filter, envelope editors
- Not a wavetable or subtractive synth
- The UI should feel like an instrument, not a DAW

---

## UI Design Direction

- **Aesthetic:** luxury, minimal, modern, aviation-inspired
- **Color palette:** near-black background, gold primary accent (#C8922A), warm off-white text
- **Feel:** a premium hardware instrument translated to software — not a DAW plugin
- **Complexity:** everything visible on one screen, no deep menus, fast to navigate
- **Reference:** the provided UI mockup (Copilot_20260415_130658.png) showing silver and dark-gold variants

---

## Out of Scope (v1.0)

- AU / AAX formats
- Windows as the **primary** release target (macOS ships first; Windows follows)
- MPE support
- Modulation matrix
- Arpeggiator / step sequencer
- Sample editing (trim, slice, normalize)
- MIDI output
- Plugin-to-plugin routing
