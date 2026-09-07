#!/usr/bin/env python3
"""Validate (and optionally conform) source samples against the AviatorKeyz mix spec.

Spec for every source sample that enters the factory bank (Analog Lab bounces etc.):

    Format         WAV (PCM)          -- never MP3 / AAC / OGG / M4A
    Sample rate    48 kHz
    Bit depth      24-bit
    Channels       Stereo (2)
    Normalization  Off                -- peaks are left where the bounce put them
    Peak level     roughly -6 .. -3 dBFS
    Dithering      Off                -- only relevant when going DOWN to 16-bit

Usage
-----
    python3 scripts/check_sample_spec.py                       # scan ContentImport/ and Resources/Factory/
    python3 scripts/check_sample_spec.py ContentImport/Bells   # scan one folder (or single files)
    python3 scripts/check_sample_spec.py --strict path ...     # non-zero exit on any deviation
    python3 scripts/check_sample_spec.py --conform path ...    # rewrite off-spec WAVs to 48k / 24-bit / stereo

`--conform` uses macOS `afconvert` (sample-rate conversion at max quality, no
dither, no gain change). Peak level is never altered: a peak outside the
-6..-3 dBFS window is reported so the bounce can be redone at the source.

Standard library only (wave/struct) so it runs on any machine in the pipeline.
"""

from __future__ import annotations

import argparse
import math
import shutil
import struct
import subprocess
import sys
import wave
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SCAN = [ROOT / "ContentImport", ROOT / "Resources" / "Factory"]

SPEC_SAMPLE_RATE = 48000
SPEC_BIT_DEPTH = 24
SPEC_CHANNELS = 2
SPEC_PEAK_MIN_DB = -6.0
SPEC_PEAK_MAX_DB = -3.0

WAV_EXTENSIONS = {".wav"}
FORBIDDEN_EXTENSIONS = {".mp3", ".aac", ".m4a", ".ogg", ".opus", ".wma", ".flac"}
AUDIO_EXTENSIONS = WAV_EXTENSIONS | FORBIDDEN_EXTENSIONS | {".aif", ".aiff"}


@dataclass
class SampleReport:
    path: Path
    ok_format: bool = False
    sample_rate: int | None = None
    bit_depth: int | None = None
    channels: int | None = None
    peak_db: float | None = None
    errors: list[str] = field(default_factory=list)    # hard spec violations
    warnings: list[str] = field(default_factory=list)  # advisory (peak window)

    @property
    def conforms(self) -> bool:
        return not self.errors

    @property
    def needs_conversion(self) -> bool:
        """True when afconvert can fix it (rate / depth / channels)."""
        return self.ok_format and any(
            e.startswith(("sample rate", "bit depth", "channels")) for e in self.errors
        )


def _peak_dbfs(path: Path, max_frames: int = 60 * 48000) -> float | None:
    """Sample peak in dBFS over up to max_frames frames (whole file for <=60 s)."""
    with wave.open(str(path), "rb") as wf:
        nch = wf.getnchannels()
        sw = wf.getsampwidth()
        n = min(wf.getnframes(), max_frames)
        raw = wf.readframes(n)

    if sw == 1:
        # 8-bit WAV is unsigned
        peak = max((abs(b - 128) for b in raw), default=0) / 128.0
    elif sw == 2:
        vals = struct.unpack(f"<{len(raw) // 2}h", raw)
        peak = max((abs(v) for v in vals), default=0) / 32768.0
    elif sw == 3:
        peak_i = 0
        for i in range(0, len(raw) - 2, 3):
            v = raw[i] | (raw[i + 1] << 8) | (raw[i + 2] << 16)
            if v & 0x800000:
                v -= 0x1000000
            av = -v if v < 0 else v
            if av > peak_i:
                peak_i = av
        peak = peak_i / 8388608.0
    elif sw == 4:
        vals = struct.unpack(f"<{len(raw) // 4}i", raw)
        peak = max((abs(v) for v in vals), default=0) / 2147483648.0
    else:
        return None

    _ = nch
    if peak <= 0.0:
        return -math.inf
    return 20.0 * math.log10(peak)


def check_file(path: Path, with_peak: bool = True) -> SampleReport:
    rep = SampleReport(path=path)
    ext = path.suffix.lower()

    if ext in FORBIDDEN_EXTENSIONS:
        rep.errors.append(f"format {ext} is not allowed for source samples (WAV only)")
        return rep
    if ext in {".aif", ".aiff"}:
        rep.errors.append("format AIFF — export as WAV")
        return rep
    if ext not in WAV_EXTENSIONS:
        rep.errors.append(f"unknown extension {ext}")
        return rep

    try:
        with wave.open(str(path), "rb") as wf:
            rep.sample_rate = wf.getframerate()
            rep.bit_depth = wf.getsampwidth() * 8
            rep.channels = wf.getnchannels()
            comp = wf.getcomptype()
            frames = wf.getnframes()
    except wave.Error as exc:
        rep.errors.append(f"not a PCM WAV the stdlib can parse ({exc}); WAVE_FORMAT_EXTENSIBLE floats are not accepted")
        return rep
    except Exception as exc:  # noqa: BLE001
        rep.errors.append(f"unreadable ({exc})")
        return rep

    rep.ok_format = True
    if comp != "NONE":
        rep.errors.append(f"compression {comp} (expected PCM)")
    if frames <= 0:
        rep.errors.append("empty file")
    if rep.sample_rate != SPEC_SAMPLE_RATE:
        rep.errors.append(f"sample rate {rep.sample_rate} Hz (spec {SPEC_SAMPLE_RATE} Hz)")
    if rep.bit_depth != SPEC_BIT_DEPTH:
        rep.errors.append(f"bit depth {rep.bit_depth}-bit (spec {SPEC_BIT_DEPTH}-bit)")
    if rep.channels != SPEC_CHANNELS:
        rep.errors.append(f"channels {rep.channels} (spec stereo)")

    if with_peak and frames > 0 and comp == "NONE":
        try:
            rep.peak_db = _peak_dbfs(path)
        except Exception as exc:  # noqa: BLE001
            rep.warnings.append(f"peak analysis failed ({exc})")
        if rep.peak_db is not None:
            if rep.peak_db == -math.inf:
                rep.errors.append("silent file")
            elif rep.peak_db > SPEC_PEAK_MAX_DB + 0.05:
                rep.warnings.append(
                    f"peak {rep.peak_db:+.1f} dBFS is hotter than {SPEC_PEAK_MAX_DB:+.0f} dBFS"
                    + (" (was this normalized?)" if rep.peak_db > -1.05 else "")
                )
            elif rep.peak_db < SPEC_PEAK_MIN_DB - 0.05:
                rep.warnings.append(f"peak {rep.peak_db:+.1f} dBFS is quieter than {SPEC_PEAK_MIN_DB:+.0f} dBFS")
    return rep


def iter_audio_files(targets: list[Path]):
    for t in targets:
        if t.is_file():
            if t.suffix.lower() in AUDIO_EXTENSIONS:
                yield t
        elif t.is_dir():
            for p in sorted(t.rglob("*")):
                if p.is_file() and not p.name.startswith(".") and p.suffix.lower() in AUDIO_EXTENSIONS:
                    yield p


def conform_with_afconvert(src: Path, dst: Path | None = None) -> None:
    """Rewrite src as 48 kHz / 24-bit / stereo PCM WAV with no dither and no gain change."""
    afconvert = shutil.which("afconvert")
    if afconvert is None:
        raise RuntimeError("afconvert not found (macOS only). Convert with your DAW / SoX instead.")
    out = dst or src
    tmp = out.with_suffix(".conform.tmp.wav")
    cmd = [
        afconvert,
        "-f", "WAVE",
        "-d", f"LEI{SPEC_BIT_DEPTH}@{SPEC_SAMPLE_RATE}",
        "-c", str(SPEC_CHANNELS),
        "-r", "127",            # best sample-rate converter quality
        "--src-complexity", "bats",
        str(src), str(tmp),
    ]
    subprocess.run(cmd, check=True, capture_output=True)
    tmp.replace(out)


def format_report(rep: SampleReport, root: Path = ROOT) -> str:
    try:
        rel = rep.path.relative_to(root)
    except ValueError:
        rel = rep.path
    status = "OK  " if rep.conforms and not rep.warnings else ("WARN" if rep.conforms else "FAIL")
    meta = ""
    if rep.ok_format:
        meta = f"{rep.sample_rate}Hz/{rep.bit_depth}b/{rep.channels}ch"
        if rep.peak_db is not None and rep.peak_db != -math.inf:
            meta += f" peak {rep.peak_db:+.1f}dBFS"
    lines = [f"{status} {rel}  {meta}".rstrip()]
    for e in rep.errors:
        lines.append(f"       - {e}")
    for w in rep.warnings:
        lines.append(f"       ~ {w}")
    return "\n".join(lines)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("paths", nargs="*", type=Path, help="files or folders (default: ContentImport + Resources/Factory)")
    parser.add_argument("--strict", action="store_true", help="exit 1 if any file deviates from the spec")
    parser.add_argument("--conform", action="store_true",
                        help="rewrite off-spec WAVs in place to 48 kHz / 24-bit / stereo (afconvert, no dither, no gain)")
    parser.add_argument("--no-peak", action="store_true", help="skip peak analysis (faster)")
    parser.add_argument("--quiet", action="store_true", help="only print deviations and the summary")
    args = parser.parse_args(argv)

    targets = [p.resolve() for p in args.paths] if args.paths else [p for p in DEFAULT_SCAN if p.exists()]
    files = list(iter_audio_files(targets))
    if not files:
        print("No audio files found.")
        return 0

    total = ok = warned = failed = converted = 0
    for f in files:
        rep = check_file(f, with_peak=not args.no_peak)
        if args.conform and rep.needs_conversion:
            try:
                conform_with_afconvert(f)
                converted += 1
                rep = check_file(f, with_peak=not args.no_peak)
                rep.warnings.insert(0, "converted in place")
            except Exception as exc:  # noqa: BLE001
                rep.errors.append(f"conform failed: {exc}")
        total += 1
        if rep.conforms and not rep.warnings:
            ok += 1
            if not args.quiet:
                print(format_report(rep))
        elif rep.conforms:
            warned += 1
            print(format_report(rep))
        else:
            failed += 1
            print(format_report(rep))

    print()
    print(f"Spec: WAV PCM · {SPEC_SAMPLE_RATE // 1000} kHz · {SPEC_BIT_DEPTH}-bit · stereo · "
          f"peak {SPEC_PEAK_MIN_DB:+.0f}..{SPEC_PEAK_MAX_DB:+.0f} dBFS · no normalization / dither")
    print(f"{total} file(s): {ok} on spec, {warned} on spec with peak warnings, {failed} off spec"
          + (f", {converted} converted" if converted else ""))
    if failed and not args.conform:
        print("Fix rate/depth/channels with:  python3 scripts/check_sample_spec.py --conform <paths>")
    if args.strict and (failed or warned):
        return 1
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
