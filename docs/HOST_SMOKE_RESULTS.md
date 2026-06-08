# Host smoke test results — 2026-06-06

Automated checks run as part of plan implementation.

## Automated (passed)

| Check | Result |
|-------|--------|
| Release build (`./scripts/build_macos.sh`) | PASS |
| C++ unit tests (64 tests) | PASS |
| Python tests (103 tests, 5 skipped) | PASS |
| Factory preset validation (114 presets, 111 WAVs) | PASS |
| `verify_release.sh` preset gate | PASS |

## Manual DAW (requires user)

These cannot be fully automated in CI without a licensed DAW session:

- T-M1-01–08 — MIDI playback, polyphony, sustain, buffer/SR changes
- T-DSP-01–03 — DAW scan, preset change under playback, source_blend automation
- T-M5-01–05 — Multi-instance, transport, bypass, CPU profile, 48h soak

Run in your primary DAW using the installed VST3:

```bash
./scripts/build_macos.sh install
```

See [TESTPLAN.md](../TESTPLAN.md) and [M5_CERTIFICATION.md](M5_CERTIFICATION.md).
