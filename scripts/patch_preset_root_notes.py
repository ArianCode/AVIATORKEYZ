#!/usr/bin/env python3
"""Add or refresh rootNote on factory preset XML from filename heuristics."""

from __future__ import annotations

import xml.etree.ElementTree as ET
from pathlib import Path

from generate_factory_assets import infer_root_note_midi

ROOT = Path(__file__).resolve().parents[1]
PRESETS = ROOT / "Resources" / "Presets" / "Factory"


def main() -> int:
    updated = 0
    for path in sorted(PRESETS.glob("*/*.xml")):
        tree = ET.parse(path)
        root = tree.getroot()
        name = root.get("name", path.stem)
        sample_id = root.get("sampleId", path.stem)
        midi = infer_root_note_midi(f"{name} {sample_id}")
        if root.get("rootNote") == str(midi):
            continue
        root.set("rootNote", str(midi))
        tree.write(path, encoding="UTF-8", xml_declaration=True)
        updated += 1
    print(f"Patched rootNote on {updated} preset(s).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
