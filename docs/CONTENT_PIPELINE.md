# Content pipeline — factory audio and presets

## Decision (v1.0)

- **One embedded WAV per category** (`factory_leads`, `factory_pads`, …) plus `factory_default`.
- Each factory preset XML sets `sampleId` on the `<Preset>` root to select which WAV loads.
- Replace placeholder WAVs in `Resources/Factory/` with licensed recordings before commercial ship.

## File layout

```
Resources/
  Factory/
    factory_default.wav
    factory_leads.wav
    factory_brass.wav
    … (one per category)
  Presets/Factory/
    Leads/Init.xml
    Pads/Pad_Warm.xml
    … (5 presets × 10 categories = 50)
```

## Preset XML format

```xml
<Preset category="Pads" name="Pad Warm" schemaVersion="1"
        sampleId="factory_pads" author="AviatorKeyz">
  <AviatorKeyzState stateVersion="1">
    <PARAM id="smear" value="0.35"/>
    …
  </AviatorKeyzState>
</Preset>
```

`sampleId` must match a file `Resources/Factory/<sampleId>.wav`.

## Build embedding

CMake globs all `Resources/Factory/*.wav` and `Resources/Presets/Factory/**/*.xml` into `AviatorKeyzBinary` via `juce_add_binary_data`.

Runtime lookup: `FactoryResources` scans `BinaryData::originalFilenames` — no hardcoded preset list in C++.

## Authoring workflow

1. Run `python3 scripts/generate_factory_assets.py` (placeholders) or hand-edit XML.
2. Drop replacement WAVs into `Resources/Factory/` keeping filenames.
3. Reconfigure/build so BinaryData regenerates.
4. Validate: `python3 scripts/validate_factory_presets.py`

## User import (post-v1 roadmap)

`SampleLibrary::loadSample()` loads disk WAV/AIFF into a sample map for user content; factory content stays embedded.

## Source sample spec (enforced)

Every WAV that enters `ContentImport/<Category>/` — Analog Lab bounces
included — must match this spec. `scripts/import_factory_bank.py` refuses
off-spec sources (rate / depth / channels are auto-conformed first).

| Setting | Required |
|---------|----------|
| Format | WAV (PCM). **Never MP3 / AAC / OGG / M4A** for source samples |
| Sample rate | 48 kHz |
| Bit depth | 24-bit |
| Channels | Stereo (most Analog Lab sounds) |
| Normalization | Off — leave the bounce level alone |
| Peak level | roughly −6 to −3 dBFS (reported, never altered) |
| Dithering | Off (only relevant when converting *down* to 16-bit) |

```
python3 scripts/check_sample_spec.py                    # scan ContentImport + Resources/Factory
python3 scripts/check_sample_spec.py --conform ContentImport/Bells   # 48 kHz / 24-bit / stereo via afconvert
python3 -m unittest tests.test_sample_spec
```

At load time the plugin peak-normalizes the decoded sample to −1 dBFS
(`SampleLibrary::normalizeSampleBuffer`), so the −6…−3 dBFS source window keeps
headroom in the bounce without changing in-plugin level consistency.

## Loudness targets (legacy, superseded by the spec above)

| Metric | Target |
|--------|--------|
| Peak | −6 to −3 dBFS in the source file (plugin normalizes to −1 dBFS on load) |
| Integrated loudness | -18 to -14 LUFS (category-dependent) |
| Silence | ≥ 5 ms at start/end |
