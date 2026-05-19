# AviatorKeyz — What to provide vs what to deliver

This document implements the product deliverables checklist for v1.0.

## You provide (inputs)

| Area | What to supply | Where it lands |
|------|----------------|----------------|
| **Audio** | Licensed WAV/AIFF per category (replace placeholders) | `Resources/Factory/*.wav` |
| **Presets** | Curated parameter sets (or edit generated XML) | `Resources/Presets/Factory/<Category>/` |
| **Brand** | Logo, wordmark, UI reference mockup | `Resources/Images/` (M4) |
| **Legal** | Signed EULA, sample clearance records | `licenses/` |
| **Signing** | Apple Developer ID + notarization credentials | Release pipeline — see [CODE_SIGNING.md](CODE_SIGNING.md) |
| **QA** | macOS 13+ machine + primary DAW (Logic/Ableton/Reaper) | Your test machine |

## The project delivers (outputs)

| Area | Deliverable | Status |
|------|-------------|--------|
| **Build** | `AviatorKeyz.vst3` (VST3 instrument) + Standalone | CMake + JUCE (`build_macos.sh`) |
| **DSP** | Sampler, Reverse, Glide, Smear, Tone, Reverb, Width, Pan, ADSR | `Source/DSP/` |
| **State** | APVTS, schema v1, preset XML, `sampleId` per preset | `Source/State/` |
| **Factory bank** | 11 embedded WAV + 50 factory presets | `Resources/` |
| **User presets** | Save to `~/Documents/AviatorKeyz/Presets/` (macOS) | `PresetManager` |
| **Host** | VST3 instrument category, project recall, automation | [README.md](../README.md) |

## Host note

AviatorKeyz is a **VST3 instrument**, not a native host sound bank. Install the `.vst3` bundle, rescan plugins, and recall state via the host project file.

**macOS install path:** `~/Library/Audio/Plug-Ins/VST3/AviatorKeyz.vst3`

## Regenerate factory placeholders

```bash
python3 scripts/generate_factory_assets.py
./scripts/build_macos.sh
```

## Verify before release

```bash
./scripts/verify_release.sh
```

See also [CONTENT_PIPELINE.md](CONTENT_PIPELINE.md), [MILESTONES.md](MILESTONES.md), [TESTPLAN.md](../TESTPLAN.md).

## Cofounder alignment

- [COFOUNDER_SESSION.md](COFOUNDER_SESSION.md) — meeting workbook (Owner / Date / Output per section)
- [COFOUNDER_MUST_DECIDE.md](COFOUNDER_MUST_DECIDE.md) — three decisions to lock before the next milestone
- [SOUND_AND_PRESET_SPEC.md](SOUND_AND_PRESET_SPEC.md) — factory audio + preset contract
- [factory_content_tracker.csv](factory_content_tracker.csv) — living status tracker (draft / approved / cut)
