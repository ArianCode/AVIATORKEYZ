#!/usr/bin/env python3
"""Set rootNote=60 (C4) on all factory preset XML files.

WARNING: XML-only normalization does NOT pitch-shift WAV audio.
Re-import with scripts/import_factory_bank.py for real alignment.
"""

from __future__ import annotations

import sys
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PRESETS = ROOT / "Resources" / "Presets" / "Factory"
DEFAULT_ROOT_NOTE = 60


def main() -> int:
    updated = 0
    for xml in sorted(PRESETS.rglob("*.xml")):
        tree = ET.parse(xml)
        root = tree.getroot()
        if root.tag != "Preset":
            print(f"SKIP (not Preset): {xml}", file=sys.stderr)
            continue
        current = root.get("rootNote", "")
        if current == str(DEFAULT_ROOT_NOTE):
            continue
        root.set("rootNote", str(DEFAULT_ROOT_NOTE))
        tree.write(xml, encoding="UTF-8", xml_declaration=True)
        updated += 1

    print(f"Updated {updated} preset(s) to rootNote={DEFAULT_ROOT_NOTE}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
