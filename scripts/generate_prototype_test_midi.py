#!/usr/bin/env python3
"""Generate Prototype_Test.mid — bass overlap + chromatic pattern for FL matrix QA."""

from __future__ import annotations

import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "release" / "prototype1" / "TEST" / "Prototype_Test.mid"

# Simple SMF type 0 writer (no external deps)
TICKS = 480
TEMPO_US = 500_000  # 120 BPM


def vlq(value: int) -> bytes:
    buf = [value & 0x7F]
    value >>= 7
    while value:
        buf.insert(0, (value & 0x7F) | 0x80)
        value >>= 7
    return bytes(buf)


def meta_tempo(us: int) -> bytes:
    return b"\xFF\x51\x03" + struct.pack(">I", us)[1:4]


def meta_eot() -> bytes:
    return b"\xFF\x2F\x00"


def note_on(ch: int, note: int, vel: int) -> bytes:
    return bytes([0x90 | ch, note & 0x7F, vel & 0x7F])


def note_off(ch: int, note: int) -> bytes:
    return bytes([0x80 | ch, note & 0x7F, 0x40])


def build_track(events: list[tuple[int, bytes]]) -> bytes:
    data = bytearray()
    last = 0
    for tick, msg in events:
        data += vlq(tick - last) + msg
        last = tick
    data += vlq(0) + meta_eot()
    return bytes(data)


def main() -> None:
    events: list[tuple[int, bytes]] = []
    t = 0

    # Bar 1: overlapping bass (E1) — fast retrigger
    for _ in range(8):
        events.append((t, note_on(0, 28, 100)))
        events.append((t + 60, note_off(0, 28)))
        t += 120

    t += TICKS // 2

    # Bar 2: chromatic up C3–C4 (keytrack test)
    for i, note in enumerate(range(48, 61)):
        events.append((t, note_on(0, note, 90)))
        events.append((t + TICKS // 4, note_off(0, note)))
        t += TICKS // 4

    t += TICKS // 2

    # Bar 3: sustain chord + release
    for note in (60, 64, 67):
        events.append((t, note_on(0, note, 80)))
    t += TICKS
    for note in (60, 64, 67):
        events.append((t, note_off(0, note)))

    track = build_track([(0, meta_tempo(TEMPO_US))] + events)
    header = b"MThd" + struct.pack(">IHHH", 6, 0, 1, TICKS)
    body = b"MTrk" + struct.pack(">I", len(track)) + track

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_bytes(header + body)
    print(f"Wrote {OUT} ({OUT.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
