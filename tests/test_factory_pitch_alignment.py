#!/usr/bin/env python3
"""Strict CI: factory WAVs must be pitch-normalized to C4 (MIDI 60)."""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FACTORY_DIR = ROOT / "Resources" / "Factory"
SCRIPTS = ROOT / "scripts"
sys.path.insert(0, str(SCRIPTS))

try:
    from pitch_align import (  # noqa: E402
        PITCH_TOLERANCE_SEMITONES,
        TARGET_MIDI_DEFAULT,
        analyze_wav,
        detect_fundamental_hz,
        hz_to_midi,
        read_wav_mono,
    )
    _PITCH_ALIGN_AVAILABLE = True
except ImportError:
    _PITCH_ALIGN_AVAILABLE = False

PROTECTED = {"factory_default"}


class TestFactoryPitchAlignment(unittest.TestCase):

    def setUp(self):
        if not _PITCH_ALIGN_AVAILABLE:
            self.skipTest("pitch_align dependencies missing (pip install -r requirements-dev.txt)")

    def test_factory_dir_exists(self):
        self.assertTrue(FACTORY_DIR.exists(), f"Missing {FACTORY_DIR}")

    def test_all_factory_wavs_aligned_to_c4(self):
        if not FACTORY_DIR.exists():
            self.skipTest("Factory directory missing")

        wavs = sorted(FACTORY_DIR.glob("factory_*.wav"))
        self.assertGreater(len(wavs), 0, "No factory_*.wav files found")

        failures: list[str] = []
        for wav in wavs:
            if wav.stem in PROTECTED:
                continue
            try:
                report = analyze_wav(wav, target_midi=TARGET_MIDI_DEFAULT)
            except Exception as exc:
                failures.append(f"{wav.name}: {exc}")
                continue

            midi_err = abs(report.detected_midi - TARGET_MIDI_DEFAULT)
            if midi_err > PITCH_TOLERANCE_SEMITONES:
                failures.append(
                    f"{wav.name}: detected MIDI {report.detected_midi:.2f} "
                    f"(target {TARGET_MIDI_DEFAULT}, err {midi_err:.2f} st, "
                    f"confidence {report.confidence:.2f})"
                )

        self.assertEqual(
            failures,
            [],
            "Pitch alignment failures:\n  " + "\n  ".join(failures),
        )

    def test_detect_fundamental_on_synthetic_c4(self):
        import math
        import numpy as np

        sr = 44100
        t = np.arange(int(sr * 0.3)) / sr
        hz = 440.0 * (2.0 ** ((60 - 69) / 12.0))
        mono = (0.5 * np.sin(2 * math.pi * hz * t)).astype(np.float32)
        f_hz, conf = detect_fundamental_hz(mono, sr)
        midi = hz_to_midi(f_hz)
        self.assertGreater(conf, 0.4)
        self.assertLess(abs(midi - 60), 1.0)


if __name__ == "__main__":
    unittest.main(verbosity=2)
