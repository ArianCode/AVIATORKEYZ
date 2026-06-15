# FL Studio validation — AviatorKeyz VST3 (post-ASan fix)

Use this checklist after installing a clean Release build:

```bash
./scripts/build_macos.sh install
./scripts/verify_plugin_binary.sh \
  ~/Library/Audio/Plug-Ins/VST3/AviatorKeyz.vst3/Contents/MacOS/AviatorKeyz
```

## Test host

Canonical configuration: [HOST_TEST_CONFIG.md](HOST_TEST_CONFIG.md)

| Field | Value |
|-------|-------|
| Host | FL Studio 2025 **25.2.3.4889** |
| Process | ARM64 native (`com.image-line.flstudio`) |
| macOS | 15.7.4 (24G517) |
| App | `/Applications/FL Studio 2025.app` |
| Plugin | `~/Library/Audio/Plug-Ins/VST3/AviatorKeyz.vst3` |

| Step | Action | Result | Notes |
|------|--------|--------|-------|
| 1 | Quit FL Studio completely | NOT TESTED | |
| 2 | Confirm no FL Studio process remains | NOT TESTED | `pgrep -lf FL` |
| 3 | Verify installed VST3 (sanitizer + codesign) | NOT TESTED | `./scripts/install_vst3.sh` output |
| 4 | Launch FL Studio 2025 **25.2.3.4889** (ARM64 native) | NOT TESTED | |
| 5 | Plugin rescan | NOT TESTED | Options → Manage plugins |
| 6 | Instantiate AviatorKeyz | NOT TESTED | |
| 7 | Open/close editor ≥10 times | NOT TESTED | |
| 8 | Play MIDI notes | NOT TESTED | |
| 9 | Change presets | NOT TESTED | |
| 10 | Create ≥4 instances | NOT TESTED | |
| 11 | Delete and recreate instances | NOT TESTED | |
| 12 | Save project | NOT TESTED | |
| 13 | Quit FL Studio | NOT TESTED | |
| 14 | Reopen FL Studio | NOT TESTED | |
| 15 | Reload project | NOT TESTED | |
| 16 | Confirm all instances restore | NOT TESTED | |
| 17 | Quit again; confirm clean shutdown | NOT TESTED | |

## Status

**Binary fix verified; FL Studio validation pending.**

Automated verification confirmed the installed VST3 is sanitizer-free and passes strict ad-hoc codesign after bundle finalization. Standalone launch succeeded in dev testing. FL Studio steps above require manual execution on **FL Studio 25.2.3.4889**.

## If FL Studio still crashes

1. Preserve the new `.ips` crash report.
2. Confirm whether `libclang_rt.asan_osx_dynamic.dylib` appears under Binary Images.
3. If absent, treat as a separate post-ASan crash.
4. Symbolicate AviatorKeyz frames using the matching `.dSYM`.
5. Do **not** use `ASAN_OPTIONS` or interceptor-suppression workarounds.
