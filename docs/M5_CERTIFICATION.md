# M5 — Host certification checklist

Automated gate: `./scripts/run_m5_smoke.sh`

## Automated (CI / local)

| Check | Command |
|-------|---------|
| Release build | `./scripts/build_macos.sh` |
| C++ unit tests | `cmake --build build --target AviatorKeyzTests && ./build/tests/AviatorKeyzTests_artefacts/Release/AviatorKeyzTests` |
| Python tests | `python3 -m unittest discover -s tests -p 'test_*.py' -v` |
| Factory bank | `python3 scripts/validate_factory_presets.py` |
| Standalone launch | `./scripts/run_standalone_macos.sh` |

## Manual macOS DAW (TESTPLAN.md)

- [ ] **T-M0-04** — VST3 scan; appears as instrument
- [ ] **T-M0-05** — Plugin window opens without crash
- [ ] **T-M1-01–08** — MIDI playback, polyphony, sustain, sample/buffer rate changes
- [ ] **T-DSP-01–03** — Note-on, preset change under playback, source_blend automation
- [ ] **T-M5-01** — Four simultaneous instances, independent state
- [ ] **T-M5-02** — Transport start/stop edge cases
- [ ] **T-M5-03** — Host bypass clean toggle
- [ ] **T-M5-04** — CPU < 5% at 32 voices, 44.1 kHz, 256 buffer
- [ ] **T-M5-05** — 48 h soak (no leak, no crash)

## Release signing

See [CODE_SIGNING.md](CODE_SIGNING.md). Requires Apple Developer ID — not runnable in CI without secrets.

```bash
# After Release build
./scripts/notarize_macos.sh   # template — set TEAM_ID and APP_PASSWORD env vars
```

## Sign-off

Record results in `known-host-issues.md` for any host-specific workarounds before v1.0 tag.
