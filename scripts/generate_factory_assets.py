#!/usr/bin/env python3
"""Generate factory WAV placeholders and 50+ factory preset XML files."""

from __future__ import annotations

import math
import struct
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FACTORY_WAV = ROOT / "Resources" / "Factory"
PRESETS = ROOT / "Resources" / "Presets" / "Factory"

SAMPLE_RATE = 44100
DURATION_S = 1.2

CATEGORIES = [
    ("Leads", "factory_leads", 880.0),
    ("Brass", "factory_brass", 311.0),
    ("Ensembles", "factory_ensembles", 440.0),
    ("Strings", "factory_strings", 523.25),
    ("Pads", "factory_pads", 196.0),
    ("Chords", "factory_chords", 329.63),
    ("Synths", "factory_synths", 660.0),
    ("Arps", "factory_arps", 740.0),
    ("Vocals", "factory_vocals", 392.0),
    ("Bells", "factory_bells", 1174.66),
]

PRESET_NAMES: dict[str, list[str]] = {
    "Leads": ["Init", "Apex Lead", "Glide Cut", "Reverse Stab", "Bright Solo"],
    "Brass": ["Horn Section", "Muted Brass", "Fanfare", "Soft Brass", "Reverse Brass"],
    "Ensembles": ["Full Stack", "Chamber", "Wide Ensemble", "Smear Wash", "Dark Ensemble"],
    "Strings": ["Legato Strings", "Staccato Hit", "Trem Strings", "Reverse Bow", "Air Strings"],
    "Pads": ["Pad Warm", "Cloud Pad", "Dark Pad", "Wide Pad", "Glide Pad"],
    "Chords": ["Warm Chords", "Bright Chords", "Smear Chords", "Narrow Chords", "Reverse Chords"],
    "Synths": ["Analog Glow", "Glass Synth", "Wide Synth", "Dark Synth", "Reverse Synth"],
    "Arps": ["Sparkle Arp", "Tight Arp", "Wide Arp", "Smear Arp", "Reverse Arp"],
    "Vocals": ["Vox Air", "Vox Choir", "Vox Glide", "Vox Reverse", "Vox Bright"],
    "Bells": ["Crystal Bell", "Soft Bell", "Wide Bell", "Reverse Bell", "Dark Bell"],
}

# Per-preset parameter templates (category base + preset index variation)
def preset_params(category: str, index: int) -> dict[str, str | float | int]:
    bases = {
        "Leads": dict(smear=0.0, tone=0.1, reverb_amount=0.1, glide_time=0.0, reverse=0),
        "Brass": dict(smear=0.05, tone=0.15, reverb_amount=0.2, glide_time=20.0, reverse=0),
        "Ensembles": dict(smear=0.25, tone=0.0, reverb_amount=0.35, glide_time=0.0, reverse=0),
        "Strings": dict(smear=0.15, tone=-0.1, reverb_amount=0.3, glide_time=40.0, reverse=0),
        "Pads": dict(smear=0.35, tone=-0.2, reverb_amount=0.45, glide_time=0.0, reverse=0),
        "Chords": dict(smear=0.2, tone=0.05, reverb_amount=0.25, glide_time=0.0, reverse=0),
        "Synths": dict(smear=0.1, tone=0.25, reverb_amount=0.15, glide_time=60.0, reverse=0),
        "Arps": dict(smear=0.05, tone=0.3, reverb_amount=0.1, glide_time=0.0, reverse=0),
        "Vocals": dict(smear=0.2, tone=0.0, reverb_amount=0.3, glide_time=80.0, reverse=0),
        "Bells": dict(smear=0.0, tone=0.35, reverb_amount=0.2, glide_time=0.0, reverse=0),
    }
    p = bases[category].copy()
    if "Reverse" in PRESET_NAMES[category][index]:
        p["reverse"] = 1
    if "Glide" in PRESET_NAMES[category][index]:
        p["glide_time"] = max(p.get("glide_time", 0), 120.0)
    if "Smear" in PRESET_NAMES[category][index]:
        p["smear"] = min(1.0, p.get("smear", 0) + 0.25)
    if "Dark" in PRESET_NAMES[category][index]:
        p["tone"] = -0.35
    if "Bright" in PRESET_NAMES[category][index] or "Crystal" in PRESET_NAMES[category][index]:
        p["tone"] = 0.4
    if "Wide" in PRESET_NAMES[category][index]:
        p["stereo_width"] = 1.6
    if category == "Pads":
        p["env_attack"] = 80.0
        p["env_release"] = 800.0
    else:
        p["env_attack"] = 5.0 + index * 3.0
        p["env_release"] = 150.0 + index * 40.0
    p.setdefault("input_gain", 0.0)
    p.setdefault("output_gain", -1.0 if index else 0.0)
    p.setdefault("reverb_size", 0.5 + index * 0.05)
    p.setdefault("stereo_width", 1.0 + index * 0.05)
    p.setdefault("pan", 0.0)
    return p


def write_wav(path: Path, freq_hz: float) -> None:
    n = int(SAMPLE_RATE * DURATION_S)
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "w") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SAMPLE_RATE)
        attack = int(0.02 * SAMPLE_RATE)
        release = int(0.15 * SAMPLE_RATE)
        for i in range(n):
            t = i / SAMPLE_RATE
            env = 1.0
            if i < attack:
                env = i / max(1, attack)
            elif i > n - release:
                env = max(0.0, (n - i) / max(1, release))
            # slight harmonic for category timbre
            s = math.sin(2 * math.pi * freq_hz * t)
            s += 0.15 * math.sin(2 * math.pi * freq_hz * 2 * t)
            s += 0.05 * math.sin(2 * math.pi * freq_hz * 3 * t)
            sample = int(32767 * 0.35 * env * s)
            w.writeframes(struct.pack("<h", sample))


def infer_root_note_midi(stem: str) -> int:
    """Best-effort MIDI root note (0-127) from a preset/sample filename.

    Strategy (applied in order):
      1. Explicit note+octave after a separator: _C3, _A#4, -G2, _C3_
         Only matches tokens preceded by _ or - to avoid false positives from
         model numbers like 'sb2', 'PMFC2', or chord extensions like '_add11'.
      2. Note-only token at end after separator: _C, _Cm, _Cmin, _CM
         Defaults to octave 4 (C4 = MIDI 60) when no octave digit is given.
      3. Fallback: 60 (C4).

    Previous bug: bare r"([A-G])(#?)(\\d)" matched 'B2' in 'sb2_guitar_C'
    (rootNote=47) and 'D1' in '_add11' (rootNote=26) — both wrong.
    """
    import re

    semis = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}
    text = stem.upper()

    # 1. Explicit note+octave after a separator (_C3, -A#4, _B2_, etc.)
    m = re.search(r"[_\-]([A-G])(#?)(\d)(?:[_\-\s]|$)", text)
    if m:
        letter, sharp, octave = m.group(1), m.group(2), int(m.group(3))
        midi = (octave + 1) * 12 + semis[letter]
        if sharp:
            midi += 1
        return max(0, min(127, midi))

    # 2. Note-only token after separator (_C, _Cm, _Cmin, _CM, _A, _Am, etc.)
    m = re.search(r"[_\-]([A-G])(?:MIN|MAJ|M|#)?(?:[_\-\s]|$)", text)
    if m:
        letter = m.group(1)
        return 12 * 5 + semis[letter]   # octave 4: (4+1)*12 + semitone

    return 60  # fallback


def write_preset_xml(
    path: Path,
    category: str,
    name: str,
    sample_id: str,
    params: dict,
    root_note: int | None = None,
) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if root_note is None:
        root_note = infer_root_note_midi(f"{name} {sample_id}")
    lines = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        f'<Preset category="{category}" name="{name}" schemaVersion="1" sampleId="{sample_id}" rootNote="{root_note}" author="AviatorKeyz">',
        '  <AviatorKeyzState stateVersion="1">',
        f'    <PARAM id="input_gain" value="{params.get("input_gain", 0.0)}"/>',
        f'    <PARAM id="output_gain" value="{params.get("output_gain", 0.0)}"/>',
        f'    <PARAM id="reverse" value="{params.get("reverse", 0)}"/>',
        f'    <PARAM id="glide_time" value="{params.get("glide_time", 0.0)}"/>',
        f'    <PARAM id="smear" value="{params.get("smear", 0.0)}"/>',
        f'    <PARAM id="tone" value="{params.get("tone", 0.0)}"/>',
        f'    <PARAM id="reverb_amount" value="{params.get("reverb_amount", 0.0)}"/>',
        f'    <PARAM id="reverb_size" value="{params.get("reverb_size", 0.5)}"/>',
        f'    <PARAM id="stereo_width" value="{params.get("stereo_width", 1.0)}"/>',
        f'    <PARAM id="env_attack" value="{params.get("env_attack", 5.0)}"/>',
        f'    <PARAM id="env_release" value="{params.get("env_release", 150.0)}"/>',
        f'    <PARAM id="pan" value="{params.get("pan", 0.0)}"/>',
        "  </AviatorKeyzState>",
        "</Preset>",
        "",
    ]
    path.write_text("\n".join(lines), encoding="utf-8")


def main() -> None:
    # Default + per-category factory samples (replace with licensed recordings before ship)
    write_wav(FACTORY_WAV / "factory_default.wav", 440.0)
    for _cat, sample_id, freq in CATEGORIES:
        write_wav(FACTORY_WAV / f"{sample_id}.wav", freq)

    count = 0
    for category, sample_id, _freq in CATEGORIES:
        names = PRESET_NAMES[category]
        for i, name in enumerate(names):
            safe = name.replace(" ", "_")
            out = PRESETS / category / f"{safe}.xml"
            write_preset_xml(out, category, name, sample_id, preset_params(category, i))
            count += 1

    print(f"Wrote {len(CATEGORIES) + 1} WAV files and {count} preset XML files under {ROOT / 'Resources'}")


if __name__ == "__main__":
    main()
