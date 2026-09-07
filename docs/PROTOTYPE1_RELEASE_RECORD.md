# Prototype #1 RC1 — release record

Immutable release metadata. Update only when cutting a new RC (RC2, etc.).

| Freeze item | Value |
|-------------|-------|
| Prototype label | `0.1.0-rc1` |
| Git tag | `prototype-1-rc1` |
| Git SHA | `0f04c959fd53d28e3ebcb58a8acbfbdbb143251f` |
| JUCE | `8.0.9` (FetchContent pin) |
| CMake minimum | `3.22` |
| Visual Studio | `2022` x64 (static CRT — no VC++ Redistributable on client) |
| macOS architectures | `arm64` + `x86_64` universal (`-DAVIATORKEYZ_MAC_UNIVERSAL=ON`) |
| macOS deployment target | `10.15` |
| Windows SDK | _(record on Windows build machine)_ |
| Display/product name | **Aviation** |
| Manufacturer code | **Avkz** — do not change |
| Plugin code | **Avk1** — do not change |
| VST3 categories | `Instrument\|Synth` |
| State schema | `STATE_SCHEMA_VERSION = 1` |
| Factory content | 114 XML / 111 WAV (BinaryData embedded) |
| Parameter IDs | Frozen in `Source/State/StateSchema.h` |

---

## Build commands

Step-by-step for both installers, including the RC label bump and the signing
gates: [INSTALLER_BUILD_GUIDE.md](INSTALLER_BUILD_GUIDE.md).

**Gates (macOS dev machine — run first, on the commit to be tagged):**

```bash
./scripts/run_prototype_gates.sh --strict
```

`--strict` fails on a dirty tree. Both packaging scripts refuse to run on a dirty tree
for the same reason: the recorded SHA must rebuild the exact binary the client runs.

**macOS installer (ship):**

```bash
./scripts/package_prototype_macos.sh
```

**Windows universal — x64 + x86 in one bundle (ship):**

```bat
scripts\build_windows_universal.bat
scripts\package_prototype_windows.bat
```

Intel and AMD need no separate build (both are x86-64), and neither do Windows 10 and 11.
The only split is host bitness, and both halves live in one `Aviation.vst3` bundle.
`build_windows_universal.bat x64only` drops the 32-bit half — use it only once FL 11
32-bit is confirmed absent, and record that decision in `PROTOTYPE1_TARGET_ENV.md`.

`build_windows.bat` (x64 only) and `build_windows_x86.bat` (x86 only) remain for
debugging a single architecture. Neither produces a shippable universal bundle.

---

## Artifact archive (fill after package)

Both platform artifacts must be built from the **same** tagged commit.

### Windows

| Field | Value |
|-------|-------|
| ZIP filename | `Aviation_Prototype1_<RC>_Windows.zip` |
| ZIP SHA-256 | _(from SHA256SUMS.txt)_ |
| Installer filename | `Aviation_Prototype1_<RC>_Windows_Setup.exe` |
| Installer SHA-256 | _(from the .sha256 sidecar)_ |
| Authenticode signature | **OPEN** — no code-signing certificate; SmartScreen will warn the tester |
| Built | _(date)_ |
| Architectures | _(x86_64 + x86, or x86_64 only — from VERSION.txt)_ |
| Minimum OS | Windows 10 (1607+) / Windows 11 |
| CPU | any x86-64 — Intel and AMD alike, no separate build |
| Validated hosts | FL 11 (record bitness) / FL 20 / FL 21+ / FL 25 — see [PROTOTYPE1_FL_MATRIX.md](PROTOTYPE1_FL_MATRIX.md) |
| Runtime deps | _(from dumpbin in VERSION.txt)_ |
| Windows SDK | _(record on Windows build machine)_ |

**Status: not built.** No Windows artifact has been produced yet. The macOS `.pkg` was
cut from tag `prototype-1-rc1`; the Windows artifacts must be built from that **same tag**
on a Windows machine. Until then the RC is macOS-only.

### macOS

| Field | Value |
|-------|-------|
| PKG filename | `Aviation_Prototype1_RC1_macOS.pkg` |
| SHA-256 | `360eb10a31c0988139c0ba24560d04672fd55d031937885236dbe0532710f1d8` |
| Size | 422 MB |
| Built | 2026-09-01 03:29 UTC, from tag `prototype-1-rc1` (`0f04c959fd`) |
| Architectures | `x86_64 arm64` (verified with `lipo -archs` on the pkg payload) |
| Install location | `/Library/Audio/Plug-Ins/VST3` (VST3 only — AU deliberately excluded) |
| Validated hosts | FL Studio 2025 macOS — RC1 payload instantiated from a real project, stable, no crash reports (2026-08-31) |

#### macOS distribution gates

| Gate | Status | Notes |
|------|--------|-------|
| Universal binary (arm64 + x86_64) | **PASS** | verified on pkg payload |
| VST3 signed — Developer ID **Application** | **PASS** | Arian Gholamipour (G3W6978UH7); hardened runtime + timestamp; signature survives pkg round-trip |
| PKG signed — Developer ID **Installer** | **OPEN** | no Developer ID Installer certificate in keychain — create one, then re-run packaging as RC2 |
| Notarized (`notarytool`) + stapled | **OPEN** | no notarytool credentials configured (`NOTARY_PROFILE` or Apple ID set) |
| `spctl` / `pkgutil --check-signature` pass | **OPEN** | blocked on the two gates above; client workaround documented in INSTALL_MACOS.txt |

An unsigned `.pkg` is fine for local testing but **not** frictionless for an external
client — they will hit Gatekeeper. Credentials are read from the environment only;
never commit certificates, Apple IDs, passwords, or Team IDs.

**Never modify an artifact after its SHA-256 is recorded.** Any code change becomes RC2.
