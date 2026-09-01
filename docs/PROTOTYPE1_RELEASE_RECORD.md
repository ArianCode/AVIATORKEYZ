# Prototype #1 RC1 — release record

Immutable release metadata. Update only when cutting a new RC (RC2, etc.).

| Freeze item | Value |
|-------------|-------|
| Prototype label | `0.1.0-rc1` |
| Git tag | `prototype-1-rc1` |
| Git SHA | _(filled at tag time)_ |
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

**Windows x64 (ship):**

```bat
scripts\build_windows.bat
scripts\package_prototype_windows.bat
```

**Windows x86 (only if FL 11 32-bit confirmed):**

```bat
scripts\build_windows_x86.bat
```

---

## Artifact archive (fill after package)

Both platform artifacts must be built from the **same** tagged commit.

### Windows

| Field | Value |
|-------|-------|
| ZIP filename | `Aviation_Prototype1_RC1_Windows_x64.zip` |
| SHA-256 | _(from SHA256SUMS.txt)_ |
| Built | _(date)_ |
| Validated hosts | FL 11 / FL 20 / FL 21+ / FL 25 (64-bit x64 VST3) — see [PROTOTYPE1_FL_MATRIX.md](PROTOTYPE1_FL_MATRIX.md) |
| Runtime deps | _(from dumpbin in VERSION.txt)_ |

### macOS

| Field | Value |
|-------|-------|
| PKG filename | `Aviation_Prototype1_RC1_macOS.pkg` |
| SHA-256 | _(from SHA256SUMS.txt)_ |
| Built | _(date)_ |
| Architectures | _(from `lipo -archs`; must include `x86_64` and `arm64`)_ |
| Install location | `/Library/Audio/Plug-Ins/VST3` (+ `/Components` for AU) |
| Validated hosts | _(Logic / Live / Reaper / FL 25 macOS)_ |

#### macOS distribution gates

| Gate | Status | Notes |
|------|--------|-------|
| Universal binary (arm64 + x86_64) | _TBD_ | Intel Macs cannot load an arm64-only build |
| VST3 signed — Developer ID **Application** | _TBD_ | `DEVELOPER_ID_APP` env var |
| PKG signed — Developer ID **Installer** | _TBD_ | `DEVELOPER_ID_INSTALLER` env var |
| Notarized (`notarytool`) + stapled | _TBD_ | `altool` is no longer accepted |
| `spctl` / `pkgutil --check-signature` pass | _TBD_ | |

An unsigned `.pkg` is fine for local testing but **not** frictionless for an external
client — they will hit Gatekeeper. Credentials are read from the environment only;
never commit certificates, Apple IDs, passwords, or Team IDs.

**Never modify an artifact after its SHA-256 is recorded.** Any code change becomes RC2.
