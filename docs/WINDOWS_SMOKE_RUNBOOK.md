# Windows smoke test — runbook

Goal of this pass: answer **"does Aviation build, package, install and load under
MSVC/Windows at all?"** It is deliberately *not* a release. Nothing here is tagged,
and no artifact produced by this runbook should reach the client.

Once Windows is proven, we cut `prototype-1-rc2` from a gated commit and rebuild
both platforms from that tag.

---

## 0. Prerequisites

```
winget install Microsoft.VisualStudio.2022.Community --override "--add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended"
winget install Kitware.CMake Git.Git JRSoftware.InnoSetup
```

- **Disk: keep 30 GB free.** The embedded factory bank generates ~800 MB of C++
  source and a very large static library; MSVC object files dwarf the macOS build.
- Reboot after the Visual Studio install so the toolchain lands in PATH.
- Run everything below from a **Developer Command Prompt for VS 2022** — that is
  what puts `dumpbin` and `signtool` on PATH.

## 1. Get the code

```
git clone https://github.com/ArianCode/AVIATORKEYZ.git
cd AVIATORKEYZ
git checkout phase1-stabilize
```

Confirm the identity you are about to build:

```
git log --oneline -1
git describe --tags --always --dirty
```

The describe string must **not** end in `-dirty`. Every packaging script refuses a
dirty tree, because a recorded SHA that cannot rebuild the binary is worthless.

## 2. Build (x64 only for the smoke test)

```
scripts\build_windows_universal.bat x64only
```

Skipping x86 halves the build. Only add the 32-bit half once you have confirmed
the FL 11 tester actually runs 32-bit.

**If the link stage stalls or runs out of memory**, `/LTCG` over the factory bank
is the likely cause. Reconfigure without link-time optimisation:

```
cmake -S . -B build-win-x64 -G "Visual Studio 17 2022" -A x64 -DAVIATORKEYZ_ENABLE_LTO=OFF
cmake --build build-win-x64 --config Release --parallel
```

Expected output:

```
build-win-x64\Aviation_artefacts\Release\VST3\Aviation.vst3\Contents\x86_64-win\Aviation.vst3
```

## 3. Confirm the client will not need a VC++ Redistributable

```
dumpbin /DEPENDENTS build-win-x64\Aviation_artefacts\Release\VST3\Aviation.vst3\Contents\x86_64-win\Aviation.vst3
```

The static CRT is configured in `CMakeLists.txt`, so the list should **not**
contain `VCRUNTIME140.dll` or `MSVCP140.dll`. If it does, the static runtime did
not take effect — stop and report it, because every client machine would then
need the redistributable installed.

## 4. Package

```
scripts\package_prototype_windows.bat
scripts\build_installer_windows.bat fast
```

`fast` uses quick compression — right for a smoke test, wrong for a shipped
artifact. Drop it for the real RC.

Order matters: packaging clears `dist\`, so the installer step must come second.

Produces:

```
dist\Aviation_Prototype1_RC1_Windows.zip
dist\Aviation_Prototype1_RC1_Windows_Setup.exe
```

> The `RC1` in those filenames is the script's label, not the commit. `VERSION.txt`
> records the real `git describe`, which is what identifies the build. Both get
> relabelled when RC2 is cut.

## 5. Install and load

Run the installer as administrator. SmartScreen will show
"Windows protected your PC" — expected; the prototype is unsigned.
Click **More info > Run anyway**.

Verify it landed:

```
dir "C:\Program Files\Common Files\VST3\Aviation.vst3\Contents\x86_64-win"
```

Then in FL Studio 25: Options > Manage plugins > Find more plugins > Start scan.
Aviation must appear under **Instruments**, not Effects.

## 6. What to actually test

Priority order — the first two are where this prototype is most likely to break:

1. **Rapid bass-note retrigger** — hammer low notes; listen for stuck or bleeding voices
2. **Key Track** — chromatic play across several octaves; confirm it repitches
3. Note-on/note-off, sustain pedal, transport stop / all-notes-off
4. Switch 10+ presets, exercise Reverse / Glide / Tone / macros
5. Open and close the UI ~10 times
6. Two instances, then four
7. Save the FLP, close FL completely, reopen, confirm preset + parameter state
8. Render/export and confirm the audio matches what you heard

Create a **new empty project in FL 25 on Windows**. Do not carry an `.flp` over
from FL 25 macOS, and do not open a newer FL's project in an older FL.

## 7. Report back

For anything that fails, capture: the `VERSION.txt` contents, the exact FL version
and build number, and what you did immediately before the failure. That maps
straight onto `release/prototype1/DOCS/FEEDBACK_TEMPLATE.txt`.
