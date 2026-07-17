#!/usr/bin/env python3
"""Audit factory presets for category / soundType / sampleId consistency."""

from __future__ import annotations

import csv
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PRESETS = ROOT / "Resources" / "Presets" / "Factory"

sys.path.insert(0, str(ROOT / "scripts"))
from category_sound_policy import (  # noqa: E402
    CANONICAL_CATEGORIES,
    SOUND_TYPE_NAMES,
    infer_sound_type,
    is_sample_id_compatible,
    suggest_category,
)


def main() -> int:
    out_path = ROOT / "docs" / "factory_category_audit.csv"
    rows: list[dict[str, str]] = []

    for xml in sorted(PRESETS.rglob("*.xml")):
        folder_cat = xml.parent.name
        try:
            root = ET.parse(xml).getroot()
        except ET.ParseError as exc:
            rows.append({
                "preset_file": str(xml.relative_to(ROOT)),
                "current_category": folder_cat,
                "xml_category": "",
                "preset_name": "",
                "sample_id": "",
                "sound_type": "",
                "suggested_category": "",
                "issue": f"parse error: {exc}",
            })
            continue

        xml_cat = root.get("category", "")
        name = root.get("name", "")
        sample_id = root.get("sampleId", "")
        sound_type = root.get("soundType", "")
        inferred = SOUND_TYPE_NAMES[infer_sound_type(xml_cat or folder_cat, name)]
        suggested = suggest_category(name, xml_cat or folder_cat) or ""

        issues: list[str] = []
        if folder_cat not in CANONICAL_CATEGORIES:
            issues.append("unknown folder category")
        if xml_cat != folder_cat:
            issues.append("folder/xml category mismatch")
        if sample_id and not is_sample_id_compatible(sample_id, xml_cat or folder_cat):
            issues.append("sampleId prefix mismatch")
        if not sound_type:
            issues.append("missing soundType attribute")
        elif sound_type != inferred:
            issues.append(f"soundType={sound_type} inferred={inferred}")
        if suggested and suggested != (xml_cat or folder_cat):
            issues.append(f"suggest move to {suggested}")

        if issues:
            rows.append({
                "preset_file": str(xml.relative_to(ROOT)),
                "current_category": folder_cat,
                "xml_category": xml_cat,
                "preset_name": name,
                "sample_id": sample_id,
                "sound_type": sound_type or inferred,
                "suggested_category": suggested,
                "issue": "; ".join(issues),
            })

    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(
            f,
            fieldnames=[
                "preset_file",
                "current_category",
                "xml_category",
                "preset_name",
                "sample_id",
                "sound_type",
                "suggested_category",
                "issue",
            ],
        )
        writer.writeheader()
        writer.writerows(rows)

    print(f"Audit: {len(rows)} issue(s) written to {out_path.relative_to(ROOT)}")
    return 1 if rows else 0


if __name__ == "__main__":
    sys.exit(main())
