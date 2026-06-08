#!/usr/bin/env python3
"""Detect fundamental pitch and normalize WAV audio to a target MIDI root (default C4)."""

from __future__ import annotations

import math
import re
import shutil
import subprocess
import tempfile
import wave
from dataclasses import dataclass, field
from pathlib import Path

import numpy as np

TARGET_MIDI_DEFAULT = 60
CONFIDENCE_THRESHOLD = 0.55
CONFIDENCE_THRESHOLD_IMPORT = 0.33
PITCH_TOLERANCE_SEMITONES = 0.5

NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]


@dataclass
class PitchReport:
    detected_midi: float
    confidence: float
    shift_semitones: float
    filename_hint_midi: int | None
    target_midi: int
    source_path: Path
    warnings: list[str] = field(default_factory=list)

    @property
    def pitch_aligned(self) -> bool:
        return abs(self.detected_midi - self.target_midi) <= PITCH_TOLERANCE_SEMITONES

    @property
    def ok(self) -> bool:
        return (
            self.confidence >= CONFIDENCE_THRESHOLD
            and self.pitch_aligned
        )


def hz_to_midi(hz: float) -> float:
    if hz <= 0.0:
        return float(TARGET_MIDI_DEFAULT)
    return 69.0 + 12.0 * math.log2(hz / 440.0)


def fold_midi_near_target(midi: float, target: int = TARGET_MIDI_DEFAULT) -> float:
    """Fold detected pitch into the octave nearest target (handles harmonic doubling)."""
    while midi - target > 6.0:
        midi -= 12.0
    while target - midi > 6.0:
        midi += 12.0
    return midi


def midi_to_hz(midi: float) -> float:
    return 440.0 * (2.0 ** ((midi - 69.0) / 12.0))


def infer_filename_midi(stem: str) -> int | None:
    """Parse octave hints from pack filenames (_C3, _C4, Aero_C4, trailing Cm/Cmin)."""
    s = stem.strip()

    m = re.search(r"[_\-\s]([A-Ga-g])([#b]?)(\d)\b", s)
    if m:
        return _note_octave_to_midi(m.group(1), m.group(2), int(m.group(3)))

    m = re.search(r"([A-Ga-g])([#b]?)(\d)\b", s)
    if m:
        return _note_octave_to_midi(m.group(1), m.group(2), int(m.group(3)))

    if re.search(r"(?:^|[_\-\s])(?:C|Cm|Cmin|Cmaj|Cmajor)(?:$|[_\-\s]|\.wav)", s, re.I):
        return TARGET_MIDI_DEFAULT

    return None


def _note_octave_to_midi(letter: str, accidental: str, octave: int) -> int | None:
    letter = letter.upper()
    if letter not in NOTE_NAMES:
        return None
    idx = NOTE_NAMES.index(letter)
    if accidental == "#":
        idx += 1
    elif accidental.lower() == "b":
        idx -= 1
    midi = (octave + 1) * 12 + idx
    if 0 <= midi <= 127:
        return midi
    return None


def read_wav_mono(path: Path) -> tuple[np.ndarray, int]:
    with wave.open(str(path), "rb") as wf:
        nch = wf.getnchannels()
        sw = wf.getsampwidth()
        sr = wf.getframerate()
        n = wf.getnframes()
        raw = wf.readframes(n)

    if sw == 2:
        samples = np.frombuffer(raw, dtype=np.int16).astype(np.float32) / 32768.0
    elif sw == 3:
        a = np.frombuffer(raw, dtype=np.uint8).reshape(-1, 3)
        samples = (
            a[:, 0].astype(np.int32)
            + (a[:, 1].astype(np.int32) << 8)
            + (a[:, 2].astype(np.int32) << 16)
        )
        samples = (samples.astype(np.float32) - 8388608.0) / 8388608.0
    elif sw == 4:
        samples = np.frombuffer(raw, dtype=np.int32).astype(np.float32) / 2147483648.0
    else:
        raise ValueError(f"Unsupported sample width: {sw}")

    if nch > 1:
        samples = samples.reshape(-1, nch).mean(axis=1)

    return samples, sr


def write_wav_mono(path: Path, mono: np.ndarray, sr: int) -> None:
    mono = np.clip(mono, -1.0, 1.0)
    pcm = (mono * 32767.0).astype(np.int16)
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "w") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(sr)
        wf.writeframes(pcm.tobytes())


def _onset_window(mono: np.ndarray, sr: int, max_ms: float = 800.0) -> np.ndarray:
    max_samples = min(len(mono), int(sr * max_ms / 1000.0))
    if max_samples < 256:
        return mono
    chunk = mono[:max_samples].astype(np.float64)
    env = np.abs(chunk)
    win = max(64, int(sr * 0.01))
    if len(env) <= win:
        return chunk.astype(np.float32)
    kernel = np.ones(win) / win
    smooth = np.convolve(env, kernel, mode="same")
    peak = int(np.argmax(smooth))
    start = max(0, peak - int(sr * 0.02))
    end = min(len(chunk), start + int(sr * 0.35))
    segment = chunk[start:end]
    if len(segment) < 256:
        segment = chunk
    return segment.astype(np.float32)


def _detect_with_librosa(segment: np.ndarray, sr: int) -> tuple[float, float] | None:
    try:
        import librosa

        f0 = librosa.yin(
            segment.astype(np.float32),
            fmin=65.0,
            fmax=2000.0,
            sr=sr,
        )
        voiced = f0[(f0 > 65.0) & (f0 < 2000.0) & np.isfinite(f0)]
        if voiced.size < 4:
            return None
        hz = float(np.median(voiced))
        voiced_frac = float(voiced.size) / float(max(1, f0.size))
        spread = float(np.std(voiced))
        stability = float(np.clip(1.0 - spread / max(hz, 1.0), 0.0, 1.0))
        confidence = float(np.clip(0.4 * voiced_frac + 0.6 * stability, 0.0, 0.98))
        return hz, confidence
    except Exception:
        return None


def detect_fundamental_hz(mono: np.ndarray, sr: int) -> tuple[float, float]:
    """
    Estimate fundamental via librosa YIN (preferred) or autocorrelation on the onset window.
    Returns (frequency_hz, confidence 0..1).
    """
    segment = _onset_window(mono, sr)
    yin = _detect_with_librosa(segment, sr)
    if yin is not None:
        return yin

    if len(segment) < 256 or sr <= 0:
        return midi_to_hz(TARGET_MIDI_DEFAULT), 0.0

    seg = segment - np.mean(segment)
    rms = float(np.sqrt(np.mean(seg * seg)))
    if rms < 1e-5:
        return midi_to_hz(TARGET_MIDI_DEFAULT), 0.0

    seg = seg / rms

    min_hz, max_hz = 65.0, 2000.0
    min_lag = max(2, int(sr / max_hz))
    max_lag = min(len(seg) - 1, int(sr / min_hz))
    if max_lag <= min_lag:
        return midi_to_hz(TARGET_MIDI_DEFAULT), 0.0

    corr = np.correlate(seg, seg, mode="full")
    corr = corr[len(corr) // 2 :]
    corr[:min_lag] = 0.0
    region = corr[min_lag : max_lag + 1]
    if region.size == 0:
        return midi_to_hz(TARGET_MIDI_DEFAULT), 0.0

    peak_idx = int(np.argmax(region)) + min_lag
    peak_val = float(corr[peak_idx])
    norm = float(corr[0]) if corr[0] > 1e-9 else 1.0
    confidence = float(np.clip(peak_val / norm, 0.0, 1.0))

    hz = sr / peak_idx

    # Harmonic product spectrum refinement
    try:
        n_fft = 1 << int(math.ceil(math.log2(len(seg))))
        spectrum = np.abs(np.fft.rfft(seg, n=n_fft))
        freqs = np.fft.rfftfreq(n_fft, 1.0 / sr)
        mask = (freqs >= min_hz) & (freqs <= max_hz)
        if np.any(mask):
            idx = int(np.argmax(spectrum[mask]))
            hps_hz = float(freqs[mask][idx])
            if abs(hps_hz - hz) / hz < 0.08:
                hz = 0.5 * (hz + hps_hz)
                confidence = min(1.0, confidence + 0.1)
    except Exception:
        pass

    return hz, confidence


def _shift_rubberband(mono: np.ndarray, sr: int, semitones: float, out_path: Path) -> bool:
    rb = shutil.which("rubberband")
    if rb is None:
        return False
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        inp = tmp / "in.wav"
        mid = tmp / "out.wav"
        write_wav_mono(inp, mono, sr)
        try:
            subprocess.run(
                [rb, "-q", "-p", str(semitones), str(inp), str(mid)],
                check=True,
                capture_output=True,
            )
        except (subprocess.CalledProcessError, FileNotFoundError):
            return False
        if not mid.exists():
            return False
        shifted, new_sr = read_wav_mono(mid)
        write_wav_mono(out_path, shifted, new_sr)
        return True


def _shift_librosa(mono: np.ndarray, sr: int, semitones: float) -> np.ndarray:
    import librosa

    return librosa.effects.pitch_shift(
        y=mono.astype(np.float32),
        sr=sr,
        n_steps=float(semitones),
    )


def pitch_shift_semitones(mono: np.ndarray, sr: int, semitones: float) -> np.ndarray:
    if abs(semitones) < 0.01:
        return mono
    try:
        return _shift_librosa(mono, sr, semitones)
    except Exception:
        ratio = 2.0 ** (semitones / 12.0)
        indices = np.arange(0, len(mono), ratio)
        indices = indices[indices < len(mono)].astype(int)
        stretched = mono[indices]
        target_len = len(mono)
        if len(stretched) < 2:
            return mono
        x_old = np.linspace(0.0, 1.0, len(stretched))
        x_new = np.linspace(0.0, 1.0, target_len)
        return np.interp(x_new, x_old, stretched).astype(np.float32)


def analyze_wav(path: Path, target_midi: int = TARGET_MIDI_DEFAULT) -> PitchReport:
    mono, _sr = read_wav_mono(path)
    hz, confidence = detect_fundamental_hz(mono, _sr)
    detected = fold_midi_near_target(hz_to_midi(hz), target_midi)
    hint = infer_filename_midi(path.stem)
    warnings: list[str] = []

    if hint is not None and abs(hint - detected) > 1.5:
        warnings.append(
            f"filename hint MIDI {hint} differs from detected {detected:.2f} by "
            f"{abs(hint - detected):.1f} semitones"
        )

    shift = float(target_midi) - detected
    return PitchReport(
        detected_midi=detected,
        confidence=confidence,
        shift_semitones=shift,
        filename_hint_midi=hint,
        target_midi=target_midi,
        source_path=path,
        warnings=warnings,
    )


def normalize_wav_to_target(
    src: Path,
    dst: Path,
    target_midi: int = TARGET_MIDI_DEFAULT,
    min_confidence: float = CONFIDENCE_THRESHOLD_IMPORT,
    max_iterations: int = 12,
) -> PitchReport:
    """Detect pitch, shift audio to target_midi, write mono WAV to dst."""
    mono, sr = read_wav_mono(src)
    report = analyze_wav(src, target_midi=target_midi)

    if report.confidence < min_confidence:
        hint = infer_filename_midi(src.stem)
        if hint is not None:
            report.detected_midi = float(fold_midi_near_target(float(hint), target_midi))
            report.confidence = max(report.confidence, 0.45)
            report.warnings.append(
                f"using filename MIDI hint {hint} due to low detect confidence "
                f"({report.confidence:.2f})"
            )
        else:
            raise RuntimeError(
                f"Low pitch confidence ({report.confidence:.2f} < {min_confidence}) "
                f"for {src.name}"
            )

    total_shift = 0.0
    work = mono
    if abs(report.shift_semitones) < 0.05:
        write_wav_mono(dst, work, sr)
        report.shift_semitones = 0.0
        return report

    for _ in range(max_iterations):
        shift = float(target_midi) - report.detected_midi
        if abs(shift) < 0.05:
            break
        if _shift_rubberband(work, sr, shift, dst.with_suffix(".tmp.wav")):
            tmp = dst.with_suffix(".tmp.wav")
            work, sr = read_wav_mono(tmp)
        else:
            work = pitch_shift_semitones(work, sr, shift)
            tmp = dst.with_suffix(".tmp.wav")
            write_wav_mono(tmp, work, sr)
        total_shift += shift
        report = analyze_wav(tmp, target_midi=target_midi)
        if report.pitch_aligned:
            tmp.replace(dst)
            report.shift_semitones = total_shift
            return report

    write_wav_mono(dst, work, sr)
    report = analyze_wav(dst, target_midi=target_midi)
    report.shift_semitones = total_shift

    if not report.pitch_aligned:
        raise RuntimeError(
            f"Could not align {src.name} to MIDI {target_midi}: "
            f"detected {report.detected_midi:.2f} (confidence {report.confidence:.2f})"
        )
    return report
