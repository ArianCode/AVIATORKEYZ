# Host test configuration (canonical)

Update this file when the primary QA machine or FL Studio build changes.

## macOS primary host (FL Studio)

| Field | Value |
|-------|-------|
| macOS | 15.7.4 (24G517) |
| Architecture | Apple Silicon (ARM64 native) |
| FL Studio product | FL Studio 2025 |
| FL Studio version | **25.2.3.4889** |
| FL Studio bundle ID | `com.image-line.flstudio` |
| App path | `/Applications/FL Studio 2025.app` |
| VST3 install path | `~/Library/Audio/Plug-Ins/VST3/AviatorKeyz.vst3` |
| Last verified on machine | 2026-06-14 |

All FL Studio manual validation in this repo (see [FL_STUDIO_VALIDATION.md](FL_STUDIO_VALIDATION.md), [TESTPLAN.md](../TESTPLAN.md) T-DSP-*) should be run against this build unless a newer version is recorded here first.

### Verify installed FL Studio version

```bash
defaults read "/Applications/FL Studio 2025.app/Contents/Info.plist" CFBundleShortVersionString
# Expected: 25.2.3.4889
```

```bash
pgrep -lf "FL Studio" || echo "FL Studio not running"
file "/Applications/FL Studio 2025.app/Contents/MacOS/FL Studio" 2>/dev/null || \
  find "/Applications/FL Studio 2025.app/Contents/MacOS" -maxdepth 1 -type f -perm +111 | head -1 | xargs file
```
