# Code signing (release)

M5 requires a **signed** `AviatorKeyz.vst3` for distribution on macOS. Windows signing is a secondary target.

## macOS (primary)

### Local development

Ad-hoc signing is applied automatically during local debug builds. For DAW testing on your machine, `./scripts/build_macos.sh install` is usually enough.

### Release distribution

1. Enroll in the **Apple Developer Program**.
2. Create a **Developer ID Application** certificate.
3. Sign the VST3 bundle and Standalone app:

```bash
VST3="build/AviatorKeyz_artefacts/VST3/AviatorKeyz.vst3"
codesign --force --options runtime --sign "Developer ID Application: Your Name (TEAMID)" \
  "${VST3}/Contents/MacOS/AviatorKeyz"

# Verify
codesign --verify --deep --strict --verbose=2 "${VST3}"
spctl -a -t execute -v "${VST3}"
```

4. **Notarize** with `xcrun notarytool` and staple the ticket before shipping.

Adjust inner binary path if CMake/JUCE output layout differs (`Contents/MacOS/AviatorKeyz`).

### CI secret storage

Store signing identity, Apple ID app password, and team ID in CI secrets (GitHub Actions). Never commit certificates or `.p12` files to the repository.

---

## Windows (secondary)

### Prerequisites

1. **Authenticode certificate** (EV recommended for SmartScreen reputation).
2. **signtool.exe** (Windows SDK) on the build machine.
3. Release build: `scripts\build_windows.bat`

### Sign the VST3 bundle

```batch
signtool sign /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 ^
  "build\AviatorKeyz_artefacts\Release\VST3\AviatorKeyz.vst3\Contents\x86_64-win\AviatorKeyz.vst3"
```

Verify:

```batch
signtool verify /pa /v "...\AviatorKeyz.vst3"
```

Adjust the inner path if CMake/JUCE output layout differs (`x86_64-win` vs `x86_64-win64`).
