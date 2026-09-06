#!/usr/bin/env python3
"""Tests for scripts/check_sample_spec.py — the source-sample mix spec gate."""

from __future__ import annotations

import math
import struct
import sys
import tempfile
import unittest
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

import check_sample_spec as spec  # noqa: E402


def write_wav(path: Path, *, rate=48000, bits=24, channels=2, peak=0.5, seconds=0.25, freq=440.0):
    frames = int(rate * seconds)
    sw = bits // 8
    with wave.open(str(path), "wb") as wf:
        wf.setnchannels(channels)
        wf.setsampwidth(sw)
        wf.setframerate(rate)
        out = bytearray()
        for i in range(frames):
            v = peak * math.sin(2 * math.pi * freq * i / rate)
            for _ in range(channels):
                if bits == 16:
                    out += struct.pack("<h", int(v * 32767))
                elif bits == 24:
                    iv = int(v * 8388607)
                    out += struct.pack("<i", iv)[:3]
                else:
                    out += struct.pack("<i", int(v * 2147483647))
        wf.writeframes(bytes(out))


class TestSampleSpec(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name)

    def tearDown(self):
        self.tmp.cleanup()

    def test_on_spec_file_passes_clean(self):
        p = self.dir / "good.wav"
        write_wav(p, peak=10 ** (-4.5 / 20))  # -4.5 dBFS
        rep = spec.check_file(p)
        self.assertTrue(rep.conforms, rep.errors)
        self.assertEqual(rep.warnings, [])
        self.assertEqual((rep.sample_rate, rep.bit_depth, rep.channels), (48000, 24, 2))
        self.assertAlmostEqual(rep.peak_db, -4.5, delta=0.1)

    def test_44k_16bit_mono_is_off_spec(self):
        p = self.dir / "old.wav"
        write_wav(p, rate=44100, bits=16, channels=1)
        rep = spec.check_file(p)
        self.assertFalse(rep.conforms)
        joined = " ".join(rep.errors)
        self.assertIn("sample rate 44100", joined)
        self.assertIn("bit depth 16", joined)
        self.assertIn("channels 1", joined)
        self.assertTrue(rep.needs_conversion)

    def test_hot_peak_warns_but_conforms(self):
        p = self.dir / "hot.wav"
        write_wav(p, peak=0.999)  # normalized to ~0 dBFS
        rep = spec.check_file(p)
        self.assertTrue(rep.conforms)
        self.assertTrue(any("hotter" in w for w in rep.warnings))
        self.assertTrue(any("normalized" in w for w in rep.warnings))

    def test_quiet_peak_warns(self):
        p = self.dir / "quiet.wav"
        write_wav(p, peak=10 ** (-14 / 20))
        rep = spec.check_file(p)
        self.assertTrue(rep.conforms)
        self.assertTrue(any("quieter" in w for w in rep.warnings))

    def test_compressed_formats_are_rejected(self):
        for ext in (".mp3", ".aac", ".m4a", ".ogg"):
            p = self.dir / f"bad{ext}"
            p.write_bytes(b"\x00" * 64)
            rep = spec.check_file(p)
            self.assertFalse(rep.conforms, ext)
            self.assertFalse(rep.needs_conversion, ext)
            self.assertIn("not allowed", rep.errors[0])

    def test_silent_file_fails(self):
        p = self.dir / "silent.wav"
        write_wav(p, peak=0.0)
        rep = spec.check_file(p)
        self.assertFalse(rep.conforms)
        self.assertIn("silent file", rep.errors)

    def test_cli_strict_exit_codes(self):
        good = self.dir / "good.wav"
        write_wav(good, peak=10 ** (-4 / 20))
        self.assertEqual(spec.main([str(good), "--quiet"]), 0)
        bad = self.dir / "bad.wav"
        write_wav(bad, rate=44100)
        self.assertEqual(spec.main([str(bad), "--quiet"]), 1)
        hot = self.dir / "hot.wav"
        write_wav(hot, peak=0.99)
        self.assertEqual(spec.main([str(hot), "--quiet"]), 0)
        self.assertEqual(spec.main([str(hot), "--quiet", "--strict"]), 1)

    @unittest.skipUnless(spec.shutil.which("afconvert"), "afconvert (macOS) not available")
    def test_conform_rewrites_to_spec_without_changing_peak(self):
        p = self.dir / "conv.wav"
        write_wav(p, rate=44100, bits=16, channels=1, peak=10 ** (-4 / 20))
        before = spec.check_file(p)
        spec.conform_with_afconvert(p)
        after = spec.check_file(p)
        self.assertTrue(after.conforms, after.errors)
        self.assertEqual((after.sample_rate, after.bit_depth, after.channels), (48000, 24, 2))
        self.assertAlmostEqual(after.peak_db, before.peak_db, delta=0.3)


if __name__ == "__main__":
    unittest.main(verbosity=2)
