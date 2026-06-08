#!/usr/bin/env python3
"""Tests for factory WAV files — per-preset embedded bank."""

from __future__ import annotations

import struct
import unittest
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FACTORY_DIR = ROOT / "Resources" / "Factory"

MIN_FACTORY_WAV_COUNT = 100


class TestFactoryWAVPresence(unittest.TestCase):

    def test_factory_dir_exists(self):
        self.assertTrue(FACTORY_DIR.exists(),
                        f"Resources/Factory directory is missing: {FACTORY_DIR}")

    def test_factory_default_wav_present(self):
        path = FACTORY_DIR / "factory_default.wav"
        self.assertTrue(path.exists(),
                        "factory_default.wav is required as sampler fallback")

    def test_minimum_factory_wav_count(self):
        count = len(list(FACTORY_DIR.glob("*.wav")))
        self.assertGreaterEqual(
            count, MIN_FACTORY_WAV_COUNT,
            f"Expected at least {MIN_FACTORY_WAV_COUNT} factory WAV files, found {count}",
        )

    def test_all_wavs_use_factory_prefix(self):
        bad = [p.name for p in FACTORY_DIR.glob("*.wav") if not p.stem.startswith("factory_")]
        self.assertEqual(bad, [],
                         "All factory WAV stems must start with 'factory_':\n  "
                         + "\n  ".join(bad))

    def test_no_duplicate_wav_stems(self):
        stems = [p.stem for p in FACTORY_DIR.glob("*.wav")]
        self.assertEqual(len(stems), len(set(stems)),
                         "Duplicate factory WAV stems detected")


class TestFactoryWAVValidity(unittest.TestCase):

    def _read_wav_info(self, path: Path):
        with wave.open(str(path), "rb") as wf:
            return {
                "nchannels":   wf.getnchannels(),
                "sampwidth":   wf.getsampwidth(),
                "framerate":   wf.getframerate(),
                "nframes":     wf.getnframes(),
                "compression": wf.getcomptype(),
            }

    def _read_wav_samples(self, path: Path, max_frames: int = 2048):
        with wave.open(str(path), "rb") as wf:
            nch = wf.getnchannels()
            sw  = wf.getsampwidth()
            n   = min(wf.getnframes(), max_frames)
            raw = wf.readframes(n)
        fmt = {1: "b", 2: "h", 4: "i"}.get(sw)
        if fmt is None:
            return []
        total = n * nch
        unpacked = struct.unpack(f"<{total}{fmt}", raw[:total * sw])
        return list(unpacked)

    def test_all_factory_wavs_are_valid_wav_files(self):
        bad = []
        for wav in FACTORY_DIR.glob("*.wav"):
            try:
                with wave.open(str(wav), "rb"):
                    pass
            except wave.Error as exc:
                bad.append(f"{wav.name}: {exc}")
            except Exception as exc:
                bad.append(f"{wav.name}: unexpected error: {exc}")
        self.assertEqual(bad, [], "Invalid WAV files:\n  " + "\n  ".join(bad))

    def test_all_factory_wavs_have_pcm_encoding(self):
        bad = []
        for wav in FACTORY_DIR.glob("*.wav"):
            try:
                info = self._read_wav_info(wav)
                if info["compression"] != "NONE":
                    bad.append(f"{wav.name}: compression={info['compression']} (expected NONE/PCM)")
            except Exception as exc:
                bad.append(f"{wav.name}: error reading: {exc}")
        self.assertEqual(bad, [], "Non-PCM WAV files:\n  " + "\n  ".join(bad))

    def test_all_factory_wavs_have_positive_frame_count(self):
        bad = []
        for wav in FACTORY_DIR.glob("*.wav"):
            try:
                info = self._read_wav_info(wav)
                if info["nframes"] <= 0:
                    bad.append(f"{wav.name}: 0 frames (empty audio)")
            except Exception as exc:
                bad.append(f"{wav.name}: error: {exc}")
        self.assertEqual(bad, [], "\n  ".join(bad))

    def test_all_factory_wavs_have_nonzero_audio(self):
        silent = []
        for wav in FACTORY_DIR.glob("*.wav"):
            try:
                samples = self._read_wav_samples(wav, max_frames=4096)
                if samples and all(s == 0 for s in samples):
                    silent.append(wav.name)
            except Exception:
                pass
        self.assertEqual(silent, [],
                         "Completely silent WAV files:\n  " + "\n  ".join(silent))

    def test_all_factory_wavs_have_valid_sample_rate(self):
        bad = []
        valid_rates = {22050, 44100, 48000, 88200, 96000}
        for wav in FACTORY_DIR.glob("*.wav"):
            try:
                info = self._read_wav_info(wav)
                if info["framerate"] not in valid_rates:
                    bad.append(f"{wav.name}: unusual sample rate {info['framerate']} Hz")
            except Exception as exc:
                bad.append(f"{wav.name}: {exc}")
        self.assertEqual(bad, [], "\n  ".join(bad))

    def test_all_factory_wavs_have_reasonable_duration(self):
        bad = []
        for wav in FACTORY_DIR.glob("*.wav"):
            try:
                info = self._read_wav_info(wav)
                duration = info["nframes"] / max(info["framerate"], 1)
                if duration < 0.05:
                    bad.append(f"{wav.name}: very short ({duration:.3f}s)")
                if duration > 120.0:
                    bad.append(f"{wav.name}: very long ({duration:.1f}s)")
            except Exception as exc:
                bad.append(f"{wav.name}: {exc}")
        self.assertEqual(bad, [], "\n  ".join(bad))

    def test_all_wavs_have_supported_channel_count(self):
        bad = []
        for wav in FACTORY_DIR.glob("*.wav"):
            try:
                info = self._read_wav_info(wav)
                if info["nchannels"] not in (1, 2):
                    bad.append(f"{wav.name}: {info['nchannels']} channels")
            except Exception as exc:
                bad.append(f"{wav.name}: {exc}")
        self.assertEqual(bad, [], "\n  ".join(bad))

    def test_all_wavs_have_standard_bit_depth(self):
        bad = []
        for wav in FACTORY_DIR.glob("*.wav"):
            try:
                info = self._read_wav_info(wav)
                bits = info["sampwidth"] * 8
                if bits not in (16, 24, 32):
                    bad.append(f"{wav.name}: {bits}-bit")
            except Exception as exc:
                bad.append(f"{wav.name}: {exc}")
        self.assertEqual(bad, [], "\n  ".join(bad))


if __name__ == "__main__":
    unittest.main(verbosity=2)
