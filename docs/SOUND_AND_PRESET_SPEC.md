# Sound & preset spec — AviatorKeyz v1.0

**Version:** 1.0  
**Status:** Draft for cofounder sign-off — fill **Approved by** / **Date** at bottom.

This is the **1-page content contract** for factory audio and presets. Technical pipeline: [CONTENT_PIPELINE.md](CONTENT_PIPELINE.md).

---

## 1) Engine fit (what “assets” means)

| Item | v1.0 decision |
|------|----------------|
| Architecture | **Hybrid:** sample-based playback + signature DSP (Reverse, Glide, Smear, Tone) |
| Embedded audio | **11 WAV files** — one per category + `factory_default` |
| Presets | **50 XML factory presets** — parameter sets + `sampleId` per preset |
| One-shot library | **Out of scope for v1.0** — do not build a separate one-shot bank unless product scope changes |
| User import | Disk WAV/AIFF via `SampleLibrary` — **confirm in cofounder scope** if v1.0 or later |

Presets are the primary way users experience variety; category WAVs define the **timbre family** for that browser section.

---

## 2) Categories and targets

| Category | sampleId | Factory WAV | Preset target |
|----------|----------|-------------|---------------|
| (fallback) | `factory_default` | `factory_default.wav` | — |
| Leads | `factory_leads` | `factory_leads.wav` | 5 |
| Brass | `factory_brass` | `factory_brass.wav` | 5 |
| Ensembles | `factory_ensembles` | `factory_ensembles.wav` | 5 |
| Strings | `factory_strings` | `factory_strings.wav` | 5 |
| Pads | `factory_pads` | `factory_pads.wav` | 5 |
| Chords | `factory_chords` | `factory_chords.wav` | 5 |
| Synths | `factory_synths` | `factory_synths.wav` | 5 |
| Arps | `factory_arps` | `factory_arps.wav` | 5 |
| Vocals | `factory_vocals` | `factory_vocals.wav` | 5 |
| Bells | `factory_bells` | `factory_bells.wav` | 5 |

**Total:** 11 WAVs + 50 presets (current repo layout).

---

## 3) What to record (per category WAV)

Each `factory_*.wav` should be:

- **Playable across MIDI range** — stable loop or long sustain; avoid clicks at loop points if looped
- **Category-defining** — the “hero” texture for that browser tab (not a generic click)
- **Single root** — document root MIDI note in tracker (default **60 / C3** unless creative choice)
- **Mono or stereo** — stereo allowed; keep phase correlation sensible for Width control

**Not required for v1.0**

- Separate one-shots per note
- Melodic phrase loops inside the product
- Round-robin or multi-sample velocity layers

**Marketing-only (optional)**

- Short melody loops, social clips, before/after demos — store outside `Resources/Factory/`

---

## 4) Rights and provenance

- **Only** audio you may **embed and redistribute** in a commercial VST3 binary
- Allowed sources: original recordings, synthesized material you own, WFH with assignment, licenses that **explicitly** permit plugin embedding
- **Forbidden:** Generic “royalty-free” packs unless license allows redistribution inside software instruments
- Record each file in [licenses/SAMPLE_CLEARANCE_CHECKLIST.md](../licenses/SAMPLE_CLEARANCE_CHECKLIST.md) and [factory_content_tracker.csv](factory_content_tracker.csv)

---

## 5) Technical standards

| Metric | Target |
|--------|--------|
| Format | WAV, 44.1 kHz or 48 kHz (consistent across factory set) |
| Bit depth | 24-bit preferred; 16-bit minimum |
| Peak | ≤ **-1.0 dBFS** |
| Integrated loudness | **-18 to -14 LUFS** (category-dependent; pads warmer, leads slightly hotter) |
| Silence | ≥ **5 ms** fade at start/end; no DC offset |
| Tuning | **A4 = 440 Hz** unless documented otherwise |
| Filename | Must match `sampleId` + `.wav` in `Resources/Factory/` |

Validation after changes:

```bash
python3 scripts/validate_factory_presets.py
./scripts/verify_release.sh
```

---

## 6) Preset authoring rules

- XML schema: `schemaVersion="1"`, `sampleId` on `<Preset>` root
- `sampleId` must exist as `Resources/Factory/<sampleId>.wav`
- Showcase signature controls across the set (Reverse, Glide, Smear, Tone) — not all at max
- Naming: human-readable `name` attribute; file name Pascal_Snake matching category folder

Authoring workflow: [CONTENT_PIPELINE.md](CONTENT_PIPELINE.md)

---

## 7) Storage and versioning

**Canonical paths (repo)**

```
Resources/Factory/           # shipped WAVs (embedded in binary)
Resources/Presets/Factory/   # shipped preset XML
```

**Optional external workspace (cloud)** — mirror structure:

```
AviatorKeyz_Content/
  Factory_v1/          # approved, ready to copy into repo
  WIP/                 # experiments, not cleared
  Rejected/            # cut from v1.0
  Marketing_only/      # loops for social — not embedded
```

**Freeze rule:** On v1.0 cut date, tag repo + lock tracker rows to `approved`. Post-cut changes ship in v1.0.1 or expansion pack.

**Living tracker:** [factory_content_tracker.csv](factory_content_tracker.csv)

---

## 8) Roles

| Task | Owner (fill in) |
|------|-----------------|
| Creative brief & references | |
| Record / design category WAVs | |
| Preset curation & naming | |
| Final sonic approval for v1.0 | |
| Technical ingest & build verify | |
| Clearance / legal | |

---

## 9) Workflow timeline (recommended)

1. Sign off this spec + creative brief  
2. Produce **WIP** WAVs → weekly review  
3. Drop approved WAVs into `Resources/Factory/` (keep filenames)  
4. Tune presets per category; validate XML  
5. Complete clearance checklist  
6. Factory freeze on date: _______________

---

**Approved by:** _______________  
**Date:** _______________
