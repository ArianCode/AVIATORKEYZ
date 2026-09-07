# Installer build guide — macOS `.pkg` and Windows `Setup.exe`

End-to-end procedure for turning a tagged commit into the two client-facing
installers. It strings together the scripts that already exist; it does not
replace them. Read this once top to bottom, then work from the checklists.

| Platform | Ship artifact | Built by | Installs to |
|----------|---------------|----------|-------------|
| macOS | `dist/Aviation_Prototype1_<RC>_macOS.pkg` | `scripts/package_prototype_macos.sh` | `/Library/Audio/Plug-Ins/VST3/Aviation.vst3` |
| Windows | `dist/Aviation_Prototype1_<RC>_Windows_Setup.exe` | `scripts/build_installer_windows.bat` | `C:\Program Files\Common Files\VST3\Aviation.vst3` |
| Windows (fallback) | `dist/Aviation_Prototype1_<RC>_Windows.zip` | `scripts/package_prototype_windows.bat` | manual copy, see `INSTALL_WINDOWS.txt` |

Both platforms ship **VST3 only**. AU is built but deliberately not packaged
(`--with-au` opts in). The Standalone app is a dev tool and is never shipped.

Every script refuses a dirty tree. A recorded SHA that cannot rebuild the
binary is worthless, so commit first, always.

---

## 0. Decide the RC label

`prototype-1-rc1` is already tagged and its `.pkg` SHA is recorded in
[PROTOTYPE1_RELEASE_RECORD.md](PROTOTYPE1_RELEASE_RECORD.md). Any commit after
that tag is a **new RC**. The label is hard-coded in four places and they must
move together, or the artifact will claim a version its binary does not carry:

| File | Setting | RC1 value |
|------|---------|-----------|
| `cmake/AviatorKeyzBuildInfo.cmake` | `AVIATORKEYZ_PROTOTYPE_VERSION` | `0.1.0-rc1` |
| `scripts/package_prototype_macos.sh` | `PKG_NAME`, `OUT_DIR`, `PROTOTYPE_VERSION` | `RC1` / `0.1.0-rc1` |
| `scripts/package_prototype_windows.bat` | `RC_LABEL`, `PROTO_VERSION` | `RC1` / `0.1.0-rc1` |
| `scripts/build_installer_windows.bat` | `RC_LABEL`, `APP_VERSION` | `RC1` / `0.1.0-rc1` |

Also update the `RC1` text in `release/prototype1/README_FIRST*.txt`,
`DOCS/INSTALL_MACOS.txt`, `DOCS/INSTALL_WINDOWS.txt` and `DOCS/KNOWN_ISSUES.txt`
(those files are copied verbatim into both installers).

Do **not** change `AppId` in `scripts/installer_windows.iss`. Inno Setup uses
it to recognise an earlier install and upgrade in place. Do not change the
manufacturer / plugin codes `Avkz` / `Avk1` either; hosts store them in
project files.

Commit the label bump, then tag:

```bash
git tag -a prototype-1-rc2 -m "Aviation Prototype 1 RC2"
git push origin prototype-1-rc2
```

Both installers must be built from **that tag** on their respective machines.

---

## 1. Pre-flight gates (macOS dev machine)

Run on the commit you are about to tag, before tagging:

```bash
./scripts/run_prototype_gates.sh --strict
```

This runs the factory-content audit, a Release build, the C++ unit tests, the
Python tests, a Standalone launch, and prints the git identity that was
compiled into `BuildInfo.h`. `--strict` fails on a dirty tree.

`BuildInfo.h` is captured at **CMake configure time**. If you commit or tag
after configuring, the binary still carries the old SHA. The packaging scripts
sidestep this by configuring a fresh tree, but any manual build must be
reconfigured after the tag.

---

## 2. macOS installer

### 2.1 Machine prerequisites

- Xcode with command-line tools selected (`xcode-select -p` should print the
  Xcode path).
- CMake 3.22 or newer and Ninja. Both are on this machine today.
- An Apple Developer Program membership and, in the login keychain:
  - **Developer ID Application** certificate (signs the `.vst3`).
  - **Developer ID Installer** certificate (signs the `.pkg`).
  - Notarization credentials stored as a `notarytool` keychain profile.

Check what is present:

```bash
security find-identity -v -p codesigning | grep "Developer ID"
```

Status on this machine as of 2026-09-07: the Application certificate is
present (`Arian Gholamipour (G3W6978UH7)`). The **Installer** certificate is
**missing**, and no notarytool profile is stored. See 2.5 to close those gates.

### 2.2 Set credentials in the shell

Credentials come from the environment only. Never commit them.

```bash
export DEVELOPER_ID_APP="Developer ID Application: Arian Gholamipour (G3W6978UH7)"
export DEVELOPER_ID_INSTALLER="Developer ID Installer: Arian Gholamipour (G3W6978UH7)"
export NOTARY_PROFILE=aviation
```

Leave any of these unset and the script still produces a `.pkg`, but reports
the missing step as an open distribution gate and writes `NO` into
`VERSION.txt`. That is fine for local testing and wrong for a client drop.

### 2.3 Build the package

```bash
git checkout prototype-1-rc2
./scripts/package_prototype_macos.sh
```

The script, in order:

1. Deletes `build-release/` and does a clean universal (`arm64` + `x86_64`)
   Release build with `AVIATORKEYZ_REQUIRE_CLEAN_TREE=ON`.
2. Confirms the binary is universal with `lipo`.
3. Signs the `.vst3` bundle with hardened runtime and a timestamp, then
   verifies the signature.
4. Stages the bundle under `/Library/Audio/Plug-Ins/VST3`, builds a component
   package with `pkgbuild`, wraps it with `productbuild`, and signs the result
   with the Installer identity.
5. Submits the `.pkg` to `notarytool`, waits, staples the ticket, validates it.
6. Writes `VERSION.txt`, copies the tester docs, and hashes the `.pkg` into
   `dist/SHA256SUMS.txt`.

Expect the build step to take a while; the factory bank is embedded as
BinaryData and both architectures are compiled. Use `--skip-build` only to
re-package an existing `build-release/` from the same commit.

### 2.4 Verify the artifact

All four should pass before the `.pkg` leaves the machine:

```bash
pkgutil --check-signature dist/Aviation_Prototype1_RC2_macOS.pkg
```

```bash
spctl --assess --type install -vv dist/Aviation_Prototype1_RC2_macOS.pkg
```

```bash
xcrun stapler validate dist/Aviation_Prototype1_RC2_macOS.pkg
```

```bash
shasum -a 256 -c dist/SHA256SUMS.txt
```

Then do a real install on a Mac that has never seen the plugin, ideally a
fresh user account: double-click the `.pkg`, confirm no Gatekeeper dialog,
open the host, rescan, load Aviation, and compare the status-bar version with
`VERSION.txt`.

### 2.5 Closing the open macOS gates

**Developer ID Installer certificate.** In Xcode, open Settings > Accounts,
select the team, click Manage Certificates, then the plus button, and choose
Developer ID Installer. It lands in the login keychain and `security
find-identity -v` will list it. Re-run the packaging script afterwards.

**Notarization profile.** Create an app-specific password at
appleid.apple.com, then store it once:

```bash
xcrun notarytool store-credentials aviation --apple-id "<apple id email>" --team-id G3W6978UH7
```

The command prompts for the app-specific password and saves it in the
keychain under the profile name `aviation`, which is what `NOTARY_PROFILE`
refers to. The packaging script only notarizes when the Installer identity is
also set, because Apple rejects unsigned packages.

---

## 3. Windows installer

Build on a Windows 10 or 11 machine. Everything below runs from a
**Developer Command Prompt for VS 2022**, which is what puts `dumpbin` and
`signtool` on `PATH`. Plain PowerShell will not find them.

### 3.0 No Windows machine? Build it on GitHub Actions

`.github/workflows/windows-build.yml` runs sections 3.3 to 3.5 on a hosted
Windows runner, calling the same scripts. It is the practical path from a Mac:
push, then download the artifact.

- Every push to `main` or a `claude/**` branch builds the **x64** bundle and
  uploads `Aviation_Prototype1_<RC>_Windows.zip`.
- Pushing a `prototype-*` tag also builds the x86 half and `Setup.exe`.
- Actions tab > Windows build > Run workflow lets you pick architectures, LTO
  and the installer by hand on any branch.

Artifacts land under the run's Summary page and expire after 30 days. Download,
unzip, and copy `VST3\Aviation.vst3` to `C:\Program Files\Common Files\VST3\`
on the test machine.

Two differences from a local build, both deliberate:

- **LTO is OFF by default** in CI. `/LTCG` over the embedded factory bank is the
  step most likely to stall or exhaust the linker on a 4-core hosted runner. It
  costs a little runtime speed and nothing functional; re-run the workflow with
  LTO `ON` for a shipped RC if the link stage completes. Locally the scripts
  still default to `ON` — export `AVIATORKEYZ_ENABLE_LTO=OFF` to override.
- **Nothing is signed.** The runner has no Authenticode certificate, so the
  installer it produces triggers SmartScreen exactly as 3.6 describes. A signed
  RC still has to be cut on a machine holding the `.pfx`.

The workflow fails the build if the binary links `VCRUNTIME140.dll` or
`MSVCP140.dll`, which is the 3.4 check run automatically.

### 3.1 Machine prerequisites

```bat
winget install Microsoft.VisualStudio.2022.Community --override "--add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended"
winget install Kitware.CMake Git.Git JRSoftware.InnoSetup
```

- Reboot after Visual Studio installs so the toolchain lands on `PATH`.
- Keep **30 GB** free. The embedded factory bank generates roughly 800 MB of
  C++ source and MSVC object files are far larger than the macOS build.
- Inno Setup 6.3 or newer. The installer script looks in both Program Files
  folders and on `PATH` for `ISCC.exe`.
- The x86 half needs the 32-bit MSVC toolchain, which the Native Desktop
  workload includes.

### 3.2 Get the exact commit

```bat
git clone https://github.com/ArianCode/AVIATORKEYZ.git
cd AVIATORKEYZ
git checkout prototype-1-rc2
git describe --tags --always --dirty
```

The describe string must print the tag name and must **not** end in `-dirty`.

### 3.3 Build the universal bundle

```bat
scripts\build_windows_universal.bat
```

This configures and builds `build-win-x64` and `build-win-x86`, then merges
the 32-bit DLL into the x64 bundle so a single `Aviation.vst3` folder carries
both `Contents\x86_64-win` and `Contents\x86-win`. Intel and AMD, Windows 10
and 11, all use the same x64 binary. Host bitness is the only real split.

Pass `x64only` to skip the 32-bit half. Do that only once the FL 11 32-bit
tester is confirmed absent, and record the decision in
[PROTOTYPE1_TARGET_ENV.md](PROTOTYPE1_TARGET_ENV.md).

If the link stage stalls or runs out of memory, link-time optimisation over
the factory bank is the cause. Reconfigure that tree with
`-DAVIATORKEYZ_ENABLE_LTO=OFF` and rebuild; it costs a little runtime speed
and nothing functional.

### 3.4 Confirm no VC++ Redistributable is needed

```bat
dumpbin /DEPENDENTS build-win-x64\Aviation_artefacts\Release\VST3\Aviation.vst3\Contents\x86_64-win\Aviation.vst3
```

The list must **not** contain `VCRUNTIME140.dll` or `MSVCP140.dll`. The
static CRT is set in `CMakeLists.txt`; if those DLLs appear, stop and report
it, because every client machine would then need the redistributable.

### 3.5 Package, then build the installer

Order matters. Packaging clears `dist\`, so the installer must come second.

```bat
scripts\package_prototype_windows.bat
```

```bat
scripts\build_installer_windows.bat
```

The packaging script copies the bundle and tester docs into
`dist\Aviation_Prototype1_RC2_Windows\`, records `dumpbin` output and a
`VERSION.txt`, zips the folder, and hashes the ZIP. The installer script
compiles `scripts\installer_windows.iss` with Inno Setup, feeding it the git
SHA and version from the shell, and hashes the resulting `Setup.exe` into a
`.sha256` sidecar.

Pass `fast` to the installer script for quick LZMA2 compression during smoke
tests. Drop it for the shipped RC; the default `lzma2/max` takes several
minutes but roughly halves the download.

What the installer does on the tester's machine:

- Requires administrator rights (Common Files is protected).
- Deletes any earlier `Aviation.vst3` before copying, so stale files never
  linger inside the bundle folder.
- Installs to `C:\Program Files\Common Files\VST3\Aviation.vst3`.
- Optional task, unticked by default: also install to
  `C:\Program Files (x86)\Common Files\VST3` for 32-bit FL 11.
- Copies the docs, MIDI test file and `VERSION.txt` to
  `C:\Users\Public\Documents\Aviation Prototype 1\`.
- Registers an uninstaller under Settings > Apps.

### 3.6 Optional Authenticode signing

Unsigned, the installer triggers SmartScreen ("Windows protected your PC")
and the tester must click More info > Run anyway. `INSTALL_WINDOWS.txt`
already explains that, and the ZIP is offered as the warning-free fallback.

To sign, obtain an Authenticode certificate (EV for immediate SmartScreen
reputation) as a `.pfx`, then set these before running the installer script:

```bat
set SIGN_PFX=C:\path\to\cert.pfx
set SIGN_PFX_PASSWORD=<password>
set SIGN_TIMESTAMP_URL=http://timestamp.digicert.com
```

The script signs and verifies the `.exe` and only then computes the SHA-256,
so the recorded hash always matches the final bytes.

### 3.7 Verify the artifact

```powershell
Get-FileHash .\dist\Aviation_Prototype1_RC2_Windows_Setup.exe -Algorithm SHA256
```

Compare against the `.sha256` sidecar. If signed:

```bat
signtool verify /pa /v dist\Aviation_Prototype1_RC2_Windows_Setup.exe
```

Then install on a **clean** Windows machine or VM with no Visual Studio, no
CMake and no source checkout. Confirm the bundle landed:

```bat
dir "C:\Program Files\Common Files\VST3\Aviation.vst3\Contents"
```

In FL Studio: Options > Manage plugins > Find more plugins > Start scan.
Aviation must appear under Instruments, not Effects. Work through
[PROTOTYPE1_FL_MATRIX.md](PROTOTYPE1_FL_MATRIX.md), FL 25 first.

---

## 4. After both installers exist

1. Fill in the Windows and macOS tables in
   [PROTOTYPE1_RELEASE_RECORD.md](PROTOTYPE1_RELEASE_RECORD.md): filenames,
   SHA-256, build date, architectures, signing and notarization status.
2. Archive the `.pkg`, `Setup.exe`, ZIP, every `VERSION.txt`, both checksum
   files and the tag name together.
3. **Never modify an artifact after its hash is recorded.** Any code change,
   however small, is the next RC and starts again at section 0.

What the tester receives:

| macOS | Windows |
|-------|---------|
| `Aviation_Prototype1_RC2_macOS.pkg` | `Aviation_Prototype1_RC2_Windows_Setup.exe` + `.sha256` |
| `SHA256SUMS.txt` | or the ZIP + `SHA256SUMS.txt` |
| `README_FIRST.txt`, `DOCS/`, `TEST/`, `VERSION.txt` | same, plus the docs land in Public Documents on install |

---

## 5. Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| Script exits with "working tree is dirty" | Uncommitted tracked changes | Commit or stash. Untracked files are ignored. |
| Plugin status bar shows an old SHA | `BuildInfo.h` captured before the commit | Delete the build tree or reconfigure, then rebuild |
| `lipo` reports a single architecture | Reused a non-universal tree | Run the packaging script without `--skip-build` |
| `.pkg` opens with "unidentified developer" | Not signed with Developer ID Installer, or not notarized | Close the gates in 2.5; the tester workaround is right-click > Open |
| `notarytool` rejects the submission | Package unsigned, or bundle lacks hardened runtime | Ensure both `DEVELOPER_ID_*` variables were set for the same run |
| MSVC link stalls or fails with out-of-memory | LTO over the embedded factory bank | Reconfigure with `-DAVIATORKEYZ_ENABLE_LTO=OFF` |
| Installer script says `ISCC.exe` not found | Inno Setup missing or not on `PATH` | `winget install JRSoftware.InnoSetup`, reopen the prompt |
| `dumpbin` or `signtool` not found | Not in a Developer Command Prompt | Open "Developer Command Prompt for VS 2022" |
| FL 11 32-bit does not see the plugin | Bundle built with `x64only`, or 32-bit task left unticked | Rebuild without `x64only`; tick the task or copy to `Program Files (x86)` |
| Installer filename still says RC1 | Label not bumped in all four files | Section 0 |
