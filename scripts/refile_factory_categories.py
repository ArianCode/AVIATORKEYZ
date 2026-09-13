#!/usr/bin/env python3
"""Re-file factory samples + presets into the categories they actually belong to.

The factory bank grew with several tabs used as dumping grounds: 36 bass patches
sat under Brass, the real brass sat under Ensembles, and the Pads tab was almost
entirely pianos and Rhodes. This moves each affected sample to the right tab and
rewrites its preset XML to match.

A move renames `Resources/Factory/factory_<old>_<stem>.wav` to
`factory_<new>_<stem>.wav`, rewrites the preset's `category` + `sampleId`, and
relocates the preset XML under `Resources/Presets/Factory/<NewCategory>/`.
Old sample ids still resolve at runtime — FactoryResources falls back to a
stem match when the category prefix no longer lines up.

Run with no arguments for a dry run; pass --apply to perform the moves.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
SAMPLE_DIR = REPO / "Resources" / "Factory"
PRESET_DIR = REPO / "Resources" / "Presets" / "Factory"

# --- classification --------------------------------------------------------
# Matched against the stem (the part after `factory_<category>_`), in order.
# First rule that matches wins; a sample already in the target tab is skipped.

KEYS_WORDS = (
    "piano", "rhodes", "upright", "wurli", "clav", "organ",
    "electric_piano", "toy_box",
)

# Whole tokens only — "violin_quartet_padin" is a string section, not a pad.
PAD_RE = re.compile(r"(^|_)(pad|texture|soundscape|atmosphere)s?(_|$)")


def classify(category: str, stem: str) -> str | None:
    """Return the tab this sample belongs in, or None to leave it alone."""

    # A patch named as a bell is a bell even when it mentions a keyboard
    # ("bell perc key"), so Bells keeps its own naming convention.
    if category == "Bells" and stem.startswith("bell"):
        return None

    # Bass first: `brass_bass_*` and the electric bass guitars filed as strings.
    if re.search(r"(^|_)bass(_|$)", stem) and "bassoon" not in stem:
        return "Bass"

    # Plucks: patches named as plucks, not "plucked <instrument>" descriptions.
    if re.search(r"(^|_)pluck(_|$)", stem):
        return "Plucks"

    # Brass filed under Ensembles.
    if category == "Ensembles" and stem.startswith("brass_"):
        return "Brass"

    # Sustained/atmospheric material stays (or becomes) a pad even when the
    # source is a keyboard — "granular rhodes pad" is a pad, not a Rhodes.
    if PAD_RE.search(stem):
        return "Pads"

    # Keys: pianos, Rhodes/EPs, organs, clavs and the `key`/`keys` patches.
    if re.search(r"(^|_)keys?(_|$)", stem) or re.search(r"(^|_)ep(_|$)", stem):
        return "Keys"
    if any(w in stem for w in KEYS_WORDS):
        return "Keys"

    return None


# Tabs whose contents we are willing to reclassify. Arps, Phrases, Vocals and
# Bells keep their identity even when a stem mentions a keyboard — an arp
# played on a piano is still an arp.
SOURCE_TABS = {"Leads", "Pads", "Brass", "Ensembles", "Synths", "Strings", "Bells"}

# Within Bells only the plucks and the outright keyboard one-shots move; a
# "synth key bell" stays a bell.
BELLS_ALLOWED_TARGETS = {"Plucks", "Keys"}


def stem_of(sample_id: str) -> tuple[str, str]:
    """factory_pads_key_cant_go -> ("pads", "key_cant_go")"""
    body = sample_id[len("factory_"):]
    cat, _, stem = body.partition("_")
    return cat, stem


def load_presets() -> dict[str, list[Path]]:
    by_sample: dict[str, list[Path]] = defaultdict(list)
    for xml in PRESET_DIR.glob("*/*.xml"):
        text = xml.read_text(encoding="utf-8")
        m = re.search(r'sampleId="([^"]+)"', text)
        if m:
            by_sample[m.group(1)].append(xml)
    return by_sample


def build_plan() -> list[dict]:
    by_sample = load_presets()
    plan: list[dict] = []

    for wav in sorted(SAMPLE_DIR.glob("factory_*.wav")):
        sample_id = wav.stem
        cat_slug, stem = stem_of(sample_id)
        if not stem:
            continue

        category = cat_slug.capitalize()
        if category not in SOURCE_TABS:
            continue

        target = classify(category, stem)
        if target is None or target == category:
            continue
        if category == "Bells" and target not in BELLS_ALLOWED_TARGETS:
            continue

        new_id = f"factory_{target.lower()}_{stem}"
        plan.append({
            "from_category": category,
            "to_category": target,
            "old_wav": wav,
            "new_wav": SAMPLE_DIR / f"{new_id}.wav",
            "old_id": sample_id,
            "new_id": new_id,
            "presets": by_sample.get(sample_id, []),
        })

    return plan


def move_file(src: Path, dst: Path) -> None:
    """Rename, preferring `git mv` so history follows tracked files.

    Most of the bank is on disk but not yet committed, and `git mv` refuses
    those, so fall back to a plain rename.
    """
    dst.parent.mkdir(parents=True, exist_ok=True)
    done = subprocess.run(
        ["git", "mv", str(src.relative_to(REPO)), str(dst.relative_to(REPO))],
        cwd=REPO, capture_output=True)
    if done.returncode != 0:
        src.rename(dst)


def apply(plan: list[dict]) -> None:
    for move in plan:
        move_file(move["old_wav"], move["new_wav"])

        for xml in move["presets"]:
            text = xml.read_text(encoding="utf-8")
            text = text.replace(f'sampleId="{move["old_id"]}"',
                                f'sampleId="{move["new_id"]}"')
            text = re.sub(r'category="[^"]*"', f'category="{move["to_category"]}"',
                          text, count=1)
            xml.write_text(text, encoding="utf-8")

            dest = PRESET_DIR / move["to_category"] / xml.name
            if dest != xml:
                move_file(xml, dest)


# --- ContentImport ---------------------------------------------------------
# import_factory_bank.py rebuilds the whole bank from ContentImport/<Category>/,
# so the staging tree has to follow the same moves or the next import would put
# everything back where it was.

CONTENT_DIR = REPO / "ContentImport"
AUDIO_SUFFIXES = {".wav", ".aif", ".aiff", ".flac", ".mp3"}


def content_slug(stem: str) -> str:
    """Mirror of file_slug() in import_factory_bank.py."""
    s = re.sub(r"[^a-z0-9]+", "_", stem.lower().strip())
    return s.strip("_") or "sample"


def build_content_plan() -> list[tuple[Path, Path]]:
    moves: list[tuple[Path, Path]] = []
    for src in sorted(CONTENT_DIR.glob("*/*")):
        if not src.is_file() or src.name.startswith("."):
            continue
        if src.suffix.lower() not in AUDIO_SUFFIXES:
            continue

        # The staging folder still carries the tab's old name in one case.
        category = "Phrases" if src.parent.name == "Chords" else src.parent.name
        if category not in SOURCE_TABS:
            continue

        target = classify(category, content_slug(src.stem))
        if target is None or target == category:
            continue
        if category == "Bells" and target not in BELLS_ALLOWED_TARGETS:
            continue

        moves.append((src, CONTENT_DIR / target / src.name))
    return moves


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--apply", action="store_true", help="perform the moves")
    args = ap.parse_args()

    # Chords was renamed to Phrases; the staging folder never followed, which
    # import_factory_bank.py now rejects as an unknown category.
    chords = CONTENT_DIR / "Chords"
    if chords.is_dir():
        print(f"ContentImport/Chords -> ContentImport/Phrases ({len(list(chords.iterdir()))} files)")
        if args.apply:
            move_file(chords, CONTENT_DIR / "Phrases")

    content = build_content_plan()
    if content:
        grouped_content: dict[tuple[str, str], int] = defaultdict(int)
        for src, dst in content:
            grouped_content[(src.parent.name, dst.parent.name)] += 1
        print("\nContentImport staging:")
        for (a, b), n in sorted(grouped_content.items()):
            print(f"  {a} -> {b}  ({n})")
        if args.apply:
            for src, dst in content:
                move_file(src, dst)

    plan = build_plan()
    if not plan:
        print("\nnothing to re-file in Resources/Factory")
        return 0

    grouped: dict[tuple[str, str], list[dict]] = defaultdict(list)
    for move in plan:
        grouped[(move["from_category"], move["to_category"])].append(move)

    orphans = 0
    for (src, dst), moves in sorted(grouped.items()):
        print(f"\n=== {src} -> {dst}  ({len(moves)}) ===")
        for m in moves:
            tag = "" if m["presets"] else "   [NO PRESET XML]"
            if not m["presets"]:
                orphans += 1
            print(f"  {m['old_id']}{tag}")

    print(f"\ntotal: {len(plan)} samples, {orphans} without a preset xml")

    if args.apply:
        apply(plan)
        print("applied.")
    else:
        print("dry run — pass --apply to perform the moves")
    return 0


if __name__ == "__main__":
    sys.exit(main())
