#!/usr/bin/env python3
"""Validate factory preset XML count, categories, and sampleId references."""

from __future__ import annotations

import sys
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PRESETS = ROOT / "Resources" / "Presets" / "Factory"
FACTORY = ROOT / "Resources" / "Factory"

CATEGORIES = [
    "Leads", "Brass", "Ensembles", "Strings", "Pads",
    "Chords", "Synths", "Arps", "Vocals", "Bells",
]

REQUIRED_PARAMS = {
    "input_gain", "output_gain", "reverse", "glide_time", "smear", "tone",
    "reverb_amount", "reverb_size", "stereo_width", "env_attack", "env_release", "pan",
}


def main() -> int:
    errors: list[str] = []
    wav_ids = {p.stem for p in FACTORY.glob("*.wav")}

    by_cat: dict[str, list[Path]] = {c: [] for c in CATEGORIES}
    for xml in PRESETS.rglob("*.xml"):
        try:
            tree = ET.parse(xml)
            root = tree.getroot()
        except ET.ParseError as e:
            errors.append(f"{xml}: parse error {e}")
            continue
        if root.tag != "Preset":
            errors.append(f"{xml}: root must be Preset")
            continue
        cat = root.get("category", "")
        name = root.get("name", "")
        sid = root.get("sampleId", "")
        if cat not in by_cat:
            errors.append(f"{xml}: unknown category {cat}")
            continue
        if not name:
            errors.append(f"{xml}: missing name")
        if sid not in wav_ids:
            errors.append(f"{xml}: sampleId {sid!r} has no matching WAV")
        state = root.find("AviatorKeyzState")
        if state is None:
            errors.append(f"{xml}: missing AviatorKeyzState")
            continue
        ids = {p.get("id") for p in state.findall("PARAM")}
        missing = REQUIRED_PARAMS - ids
        if missing:
            errors.append(f"{xml}: missing PARAM ids {sorted(missing)}")
        by_cat[cat].append(xml)

    for cat in CATEGORIES:
        n = len(by_cat[cat])
        if n < 5:
            errors.append(f"Category {cat}: only {n} presets (need ≥5)")

    total = sum(len(v) for v in by_cat.values())
    print(f"Factory presets: {total} across {len(CATEGORIES)} categories")
    print(f"Factory WAV files: {len(wav_ids)}")

    if errors:
        print("\nERRORS:")
        for e in errors:
            print(" ", e)
        return 1

    print("OK — all factory presets valid")
    return 0


if __name__ == "__main__":
    sys.exit(main())
