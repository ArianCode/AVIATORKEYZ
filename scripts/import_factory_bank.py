#!/usr/bin/env python3
"""Import a WAV bank from ContentImport/<Category>/ into Resources/Factory + preset XML."""

from __future__ import annotations

import argparse
import re
import shutil
import sys
from pathlib import Path

# Reuse preset templates from the placeholder generator
from generate_factory_assets import (
    PRESET_NAMES,
    infer_root_note_midi,
    preset_params,
    write_preset_xml,
)

DEFAULT_ROOT_NOTE = 60  # C4 — all factory samples play at native pitch on MIDI C4
DEFAULT_SOURCE_BPM = 120.0

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE = ROOT / "ContentImport"
FACTORY_WAV = ROOT / "Resources" / "Factory"
PRESETS = ROOT / "Resources" / "Presets" / "Factory"

CANONICAL_CATEGORIES = [
    "Leads",
    "Brass",
    "Ensembles",
    "Strings",
    "Pads",
    "Chords",
    "Synths",
    "Arps",
    "Vocals",
    "Bells",
]

AUDIO_EXTENSIONS = {".wav", ".WAV"}
PROTECTED_WAV = {"factory_default"}


def category_slug(category: str) -> str:
    return category.lower()


LEGACY_CATEGORY_WAVS = {f"factory_{category_slug(c)}" for c in CANONICAL_CATEGORIES}


def file_slug(stem: str) -> str:
    s = stem.lower().strip()
    s = re.sub(r"[^a-z0-9]+", "_", s)
    return s.strip("_") or "sample"


def safe_preset_filename(display_name: str) -> str:
    return display_name.replace(" ", "_")


def make_sample_id(category: str, display_name: str) -> str:
    return f"factory_{category_slug(category)}_{file_slug(display_name)}"


def infer_source_bpm(stem: str) -> float:
    """Tempo for time-stretch. One-shots default to 120 to avoid varispeed pitch drift when layering."""
    stem_lower = stem.lower()
    loop_markers = ("loop", "arp_loop", "_loop_", "chop_loop")
    is_loop = any(m in stem_lower for m in loop_markers)

    if not is_loop:
        return DEFAULT_SOURCE_BPM

    m = re.search(r"(\d{2,3})\s*bpm", stem, re.I)
    if m:
        return float(m.group(1))

    for part in re.split(r"[_\-\s]+", stem):
        if part.isdigit():
            val = int(part)
            if 60 <= val <= 200:
                return float(val)

    return DEFAULT_SOURCE_BPM


def neutral_preset_params() -> dict[str, str | float | int]:
    return {
        "input_gain": 0.0,
        "output_gain": 0.0,
        "reverse": 0,
        "glide_time": 0.0,
        "smear": 0.0,
        "tone": 0.0,
        "reverb_amount": 0.1,
        "reverb_size": 0.5,
        "stereo_width": 1.0,
        "env_attack": 5.0,
        "env_release": 150.0,
        "pan": 0.0,
    }


def params_for_preset(category: str, display_name: str) -> dict[str, str | float | int]:
    names = PRESET_NAMES.get(category, [])
    if display_name in names:
        return preset_params(category, names.index(display_name))
    p = neutral_preset_params()
    if "Reverse" in display_name:
        p["reverse"] = 1
    if "Glide" in display_name:
        p["glide_time"] = 120.0
    if "Smear" in display_name:
        p["smear"] = 0.35
    if "Dark" in display_name:
        p["tone"] = -0.35
    if "Bright" in display_name or "Crystal" in display_name:
        p["tone"] = 0.4
    if "Wide" in display_name:
        p["stereo_width"] = 1.6
    if category == "Pads":
        p["env_attack"] = 80.0
        p["env_release"] = 800.0
    return p


def collect_import_entries(source: Path) -> list[tuple[str, Path, str, str]]:
    """Returns list of (category, src_path, display_name, sample_id)."""
    if not source.is_dir():
        raise SystemExit(f"Source directory not found: {source}")

    entries: list[tuple[str, Path, str, str]] = []
    unknown_dirs: list[str] = []

    for child in sorted(source.iterdir()):
        if not child.is_dir() or child.name.startswith("."):
            continue
        if child.name not in CANONICAL_CATEGORIES:
            unknown_dirs.append(child.name)
            continue

        for audio in sorted(child.iterdir()):
            if not audio.is_file() or audio.name.startswith("."):
                continue
            if audio.suffix not in AUDIO_EXTENSIONS:
                continue
            display_name = audio.stem
            sample_id = make_sample_id(child.name, display_name)
            entries.append((child.name, audio, display_name, sample_id))

    if unknown_dirs:
        print("ERROR: unknown category folders (must match exactly):", file=sys.stderr)
        for d in unknown_dirs:
            print(f"  {d}", file=sys.stderr)
        print("\nAllowed folder names:", file=sys.stderr)
        for c in CANONICAL_CATEGORIES:
            print(f"  {c}", file=sys.stderr)
        raise SystemExit(1)

    return entries


def clean_generated(only_categories: set[str] | None = None) -> None:
    """Remove per-preset factory WAVs and category preset XML from a prior import."""
    removed_wav = 0
    removed_xml = 0

    for wav in FACTORY_WAV.glob("factory_*.wav"):
        stem = wav.stem
        if stem in PROTECTED_WAV:
            continue
        # legacy single-segment ids: factory_leads (no extra underscore segment)
        parts = stem.split("_", 2)
        if len(parts) < 3:
            continue
        cat_part = parts[1]
        if only_categories is not None:
            slugs = {category_slug(c) for c in only_categories}
            if cat_part not in slugs:
                continue
        wav.unlink()
        removed_wav += 1

    for stem in LEGACY_CATEGORY_WAVS:
        legacy = FACTORY_WAV / f"{stem}.wav"
        if legacy.is_file():
            legacy.unlink()
            removed_wav += 1

    for category in CANONICAL_CATEGORIES:
        if only_categories is not None and category not in only_categories:
            continue
        cat_dir = PRESETS / category
        if not cat_dir.is_dir():
            continue
        for xml in cat_dir.glob("*.xml"):
            xml.unlink()
            removed_xml += 1

    print(f"Clean: removed {removed_wav} WAV(s), {removed_xml} preset XML(s)")


def run_import(
    source: Path,
    *,
    dry_run: bool = False,
    do_clean: bool = False,
    copy_only: bool = False,
) -> int:
    entries = collect_import_entries(source)
    if not entries:
        print(f"No WAV files found under {source}/<Category>/")
        print("Expected folders:", ", ".join(CANONICAL_CATEGORIES))
        return 1

    categories_touched = {e[0] for e in entries}
    if do_clean and not dry_run:
        clean_generated(categories_touched)

    mode = "copy-only" if copy_only else "pitch-normalize"
    print(
        f"{'DRY RUN — ' if dry_run else ''}Importing {len(entries)} sample(s) from {source} ({mode})"
    )
    failures = 0
    for category, src, display_name, sample_id in entries:
        wav_out = FACTORY_WAV / f"{sample_id}.wav"
        xml_out = PRESETS / category / f"{safe_preset_filename(display_name)}.xml"
        params = params_for_preset(category, display_name)
        print(f"  [{category}] {display_name!r} -> {sample_id}.wav + {xml_out.relative_to(ROOT)}")

        if dry_run:
            continue

        FACTORY_WAV.mkdir(parents=True, exist_ok=True)
        PRESETS.mkdir(parents=True, exist_ok=True)
        try:
            if copy_only:
                shutil.copy2(src, wav_out)
            else:
                from pitch_align import (  # noqa: PLC0415
                    CONFIDENCE_THRESHOLD_IMPORT,
                    normalize_wav_to_target,
                )

                report = normalize_wav_to_target(
                    src,
                    wav_out,
                    target_midi=DEFAULT_ROOT_NOTE,
                    min_confidence=CONFIDENCE_THRESHOLD_IMPORT,
                )
                for w in report.warnings:
                    print(f"    warn: {w}")
        except Exception as exc:
            print(f"ERROR: import failed for {src.name}: {exc}", file=sys.stderr)
            failures += 1
            continue

        root_note = infer_root_note_midi(src.stem)
        write_preset_xml(xml_out, category, display_name, sample_id, params, root_note=root_note)

    if failures:
        print(f"Import finished with {failures} failure(s).", file=sys.stderr)
        return 1

    if not dry_run:
        ok_count = len(entries) - failures
        print(f"Done. Wrote {ok_count} WAV(s) and preset XML under Resources/")
        print("Preset count per category = number of WAV files in ContentImport/<Category>/")
        print("Next: python3 scripts/validate_factory_presets.py && cmake --build build")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Import factory WAV bank from ContentImport/<Category>/ folders."
    )
    parser.add_argument(
        "--source",
        type=Path,
        default=DEFAULT_SOURCE,
        help=f"Import root (default: {DEFAULT_SOURCE.relative_to(ROOT)})",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print manifest without writing files",
    )
    parser.add_argument(
        "--clean",
        action="store_true",
        help="Remove prior per-preset factory_*_<slug> WAVs and category XML before import",
    )
    parser.add_argument(
        "--copy-only",
        action="store_true",
        help="Copy WAV bytes as-is (no pitch detection or shifting)",
    )
    args = parser.parse_args()

    source = args.source.resolve()
    if args.clean and args.dry_run:
        print("DRY RUN — would clean generated assets for categories found in source")
        entries = collect_import_entries(source)
        if entries:
            clean_generated({e[0] for e in entries})
        return run_import(source, dry_run=True, do_clean=False, copy_only=args.copy_only)

    return run_import(
        source,
        dry_run=args.dry_run,
        do_clean=args.clean,
        copy_only=args.copy_only,
    )


if __name__ == "__main__":
    sys.exit(main())
