# About AviatorKeyz

**AviatorKeyz** is a premium, polyphonic sample-based VST3 instrument for producers, composers, and musicians who want the polish and musicality of top-tier tools like Kontakt or Analog Lab — without the complexity of a full technical sampler.

It is a curated instrument with a strong editorial voice: factory sounds, signature creative controls, and a luxury aviation-inspired interface — all on one screen, fast to navigate.

---

## The Aim

AviatorKeyz exists to feel like **premium hardware translated to software** — an instrument you open, play, and stay in. Not a DAW inside a plugin.

The goal is simple:

1. **Inspire immediately** — 50 factory presets across 10 categories, browsable by sound family
2. **Make every preset feel alive** — four signature controls (Reverse, Glide, Smear, Tone) that reshape texture without deep sound design
3. **Stay focused** — no keygroup editing, scripting, modulation matrix, or wavetable synthesis
4. **Look and feel memorable** — a dark, gold-accented interface that reads as luxury, not utility

AviatorKeyz is deliberately **not** a general-purpose sampler. It is an editorial instrument: curated sounds, a consistent creative engine, and a brand identity you remember after one session.

---

## The Aesthetic

The visual language is **aviation-inspired luxury** — the cockpit of a private jet, not a mixing console.

| Element | Direction |
|---------|-----------|
| **Mood** | Minimal, modern, confident — premium hardware in software form |
| **Surfaces** | Piano-black and near-black backgrounds (`#06060a`–`#111111`), layered depth, subtle machined edges |
| **Accent** | Champagne / satin gold — not amber — for knobs, highlights, and active states (`#C8922A`, `#D4BC86`) |
| **Typography** | Warm off-white primary text (`#F0E6D0` / `#EDEAE1`); muted labels for hierarchy |
| **Layout** | Everything visible on one screen — no deep menus, no clutter |
| **Feel** | Thin arc knobs, pill toggles, gold trim lines, soft ambient glow — tactile and restrained |

The UI should feel like sitting down at a high-end instrument panel: dark, precise, and quietly opulent. Fast to navigate. Hard to forget.

---

## The Sound

### Preset categories

Leads · Brass · Ensembles · Strings · Pads · Chords · Synths · Arps · Vocals · Bells

### Signature controls

| Control | What it does |
|---------|--------------|
| **Reverse** | Per-voice backward playback while preserving MIDI timing |
| **Glide** | Portamento between notes (0–500 ms) |
| **Smear** | Softens transients for a washed, atmospheric texture |
| **Tone** | Single-knob tilt EQ — darker/warmer vs brighter/cleaner |

Supporting processing includes input/output gain, plate reverb, stereo width, pan, and per-voice ADSR — enough to polish, not enough to distract.

### Factory content

- **11 embedded WAV** samples (one per category plus default)
- **50 factory presets** (XML), embedded in the plugin binary
- User presets save to `Documents/AviatorKeyz/Presets/`
- User sample import: load compatible WAV/AIFF, map to MIDI, play inside the plugin

---

## At a Glance

| | |
|---|---|
| **Format** | VST3 instrument (MIDI in, stereo audio out) |
| **Platform** | macOS 13+ (primary); Windows 10/11 x64 (secondary) |
| **Primary DAWs** | Logic, Ableton, Reaper (macOS) |
| **Stack** | C++20, JUCE 8, CMake |
| **Version** | 0.1.0 (pre-release) |

---

## Development Status

| Milestone | Focus | Status |
|-----------|--------|--------|
| M0 | Scaffold & build | Complete |
| M1 | Core sampler + MIDI | Complete |
| M2 | Creative DSP (Reverse, Glide, Smear, Tone, reverb) | Complete |
| M3 | Preset system + factory bank | Complete |
| M4 | Premium UI polish | In progress |
| M5 | FL Studio certification & release signing | Planned |

Current emphasis: finishing the premium UI, replacing placeholder factory audio with cleared/licensed material, and host QA on Windows + FL Studio before v1.0.

---

## Team

| | Role |
|---|------|
| **Arian** | Lead developer — product direction, DSP, plugin architecture, UI implementation |
| **Keyz** | Brand and public presence — partnerships, content, marketing |

Legal entity, IP assignments, and sample clearance are documented under `licenses/`. This is a pre-release project; commercial distribution requires completed clearance and signing.

---

## Learn More

| Document | Purpose |
|----------|---------|
| [README.md](README.md) | Build, install, FL Studio setup |
| [docs/PRODUCT_SPEC.md](docs/PRODUCT_SPEC.md) | Product scope and v1.0 boundaries |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Modules, threading, state flow |
| [docs/DELIVERABLES.md](docs/DELIVERABLES.md) | Release inputs vs outputs |

---

*Last updated: May 2026*
