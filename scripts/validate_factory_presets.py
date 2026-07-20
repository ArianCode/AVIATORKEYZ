#!/usr/bin/env python3
"""Validate factory preset XML count, categories, sampleId references, and policy."""

from __future__ import annotations

import sys
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PRESETS = ROOT / "Resources" / "Presets" / "Factory"
FACTORY = ROOT / "Resources" / "Factory"

sys.path.insert(0, str(ROOT / "scripts"))
from category_sound_policy import (  # noqa: E402
    CANONICAL_CATEGORIES,
    is_sample_id_compatible,
    suggest_category,
)

REQUIRED_PARAMS = {
    "input_gain", "output_gain", "reverse", "glide_time", "smear", "tone",
    "reverb_amount", "reverb_size", "stereo_width", "env_attack", "env_release", "pan",
}

PLAYBACK_PARAMS = {
    "src_playback_mode", "src_keytrack", "src_loop_mode", "src_bpm_sync",
}


def main() -> int:
    errors: list[str] = []
    warnings: list[str] = []
    wav_ids = {p.stem for p in FACTORY.glob("*.wav")}
    referenced_wavs: set[str] = set()

    by_cat: dict[str, list[Path]] = {c: [] for c in CANONICAL_CATEGORIES}
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
        folder_cat = xml.parent.name
        name = root.get("name", "")
        sid = root.get("sampleId", "")
        sound_type = root.get("soundType", "")

        if cat not in by_cat:
            errors.append(f"{xml}: unknown category {cat}")
            continue
        if folder_cat != cat:
            errors.append(f"{xml}: folder {folder_cat} != XML category {cat}")
        if not name:
            errors.append(f"{xml}: missing name")
        if sid not in wav_ids:
            errors.append(f"{xml}: sampleId {sid!r} has no matching WAV")
        else:
            referenced_wavs.add(sid)
        if not is_sample_id_compatible(sid, cat):
            errors.append(f"{xml}: sampleId {sid!r} incompatible with category {cat}")
        if not sound_type:
            warnings.append(f"{xml}: missing soundType attribute (run patch_factory_playback_params.py)")

        suggested = suggest_category(name, cat)
        if suggested and suggested != cat:
            warnings.append(f"{xml}: filename suggests category {suggested} (currently {cat})")

        state = root.find("AviatorKeyzState")
        if state is None:
            errors.append(f"{xml}: missing AviatorKeyzState")
            continue
        ids = {p.get("id") for p in state.findall("PARAM")}
        missing = REQUIRED_PARAMS - ids
        if missing:
            errors.append(f"{xml}: missing PARAM ids {sorted(missing)}")
        missing_playback = PLAYBACK_PARAMS - ids
        if missing_playback:
            warnings.append(f"{xml}: missing playback PARAMs {sorted(missing_playback)}")

        by_cat[cat].append(xml)

    orphans = sorted(wav_ids - referenced_wavs - {"factory_default"})
    for orphan in orphans:
        if orphan.startswith("factory_") and orphan.count("_") >= 2:
            warnings.append(f"Orphan WAV not referenced by any preset: {orphan}.wav")

    for cat in CANONICAL_CATEGORIES:
        n = len(by_cat[cat])
        if n < 5:
            errors.append(f"Category {cat}: only {n} presets (need ≥5)")

    total = sum(len(v) for v in by_cat.values())
    print(f"Factory presets: {total} across {len(CANONICAL_CATEGORIES)} categories")
    print(f"Factory WAV files: {len(wav_ids)}")

    if warnings:
        print(f"\nWARNINGS ({len(warnings)}):")
        for w in warnings[:20]:
            print(" ", w)
        if len(warnings) > 20:
            print(f"  ... and {len(warnings) - 20} more")

    if errors:
        print("\nERRORS:")
        for e in errors:
            print(" ", e)
        return 1

    print("OK — all factory presets valid")
    return 0


if __name__ == "__main__":
    sys.exit(main())
