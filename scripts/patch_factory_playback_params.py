#!/usr/bin/env python3
"""Patch existing factory preset XML with soundType + playback PARAM nodes."""

from __future__ import annotations

import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PRESETS = ROOT / "Resources" / "Presets" / "Factory"

sys.path.insert(0, str(ROOT / "scripts"))
from category_sound_policy import (  # noqa: E402
    SOUND_TYPE_NAMES,
    infer_sound_type,
    playback_params_for,
)


def upsert_param(state: ET.Element, param_id: str, value: str | float | int) -> None:
    for child in state.findall("PARAM"):
        if child.get("id") == param_id:
            child.set("value", str(value))
            return
    node = ET.SubElement(state, "PARAM")
    node.set("id", param_id)
    node.set("value", str(value))


def patch_file(xml_path: Path) -> bool:
    tree = ET.parse(xml_path)
    root = tree.getroot()
    if root.tag != "Preset":
        return False

    category = root.get("category", xml_path.parent.name)
    name = root.get("name", xml_path.stem)
    sound_type = SOUND_TYPE_NAMES[infer_sound_type(category, name)]
    root.set("soundType", sound_type)

    state = root.find("AviatorKeyzState")
    if state is None:
        state = ET.SubElement(root, "AviatorKeyzState")
        state.set("stateVersion", "1")

    for key, value in playback_params_for(category, name).items():
        upsert_param(state, key, value)

    xml_text = ET.tostring(root, encoding="unicode")
    if not xml_text.startswith("<?xml"):
        xml_text = '<?xml version="1.0" encoding="UTF-8"?>\n' + xml_text
    xml_path.write_text(xml_text + "\n", encoding="utf-8")
    return True


def main() -> int:
    count = 0
    for xml in sorted(PRESETS.rglob("*.xml")):
        if patch_file(xml):
            count += 1
    print(f"Patched {count} preset XML file(s)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
