# Prototype #1 — target environments (Windows/FL + macOS)

**Status:** Windows-first. Build a **universal** Windows bundle; macOS follows after Windows is signed off.
**Scope:** One Windows ZIP containing a single universal `Aviation.vst3` (x86_64-win + x86-win), then a macOS `.pkg` — both from the same tagged commit.
**Minimum OS (hard floor):** Windows 10 (1607+) or Windows 11 — JUCE 8.0.9 does not support Windows 7/8.

## What "universal" means on Windows

Not the same thing as a macOS universal binary. Three axes get confused here:

| Axis | Separate build needed? | Why |
|------|:----------------------:|-----|
| **Intel vs AMD CPU** | **No** | Both are x86-64. One x64 binary runs on every desktop Intel and AMD chip. |
| **Windows 10 vs Windows 11** | **No** | Same ABI. One build, floor is Win10 1607+. |
| **64-bit vs 32-bit FL host** | **Yes** | Host bitness, not CPU. A 32-bit process cannot load a 64-bit DLL. |

Only the third axis produces a second binary, and a VST3 bundle can carry both
architecture folders at once, so it still ships as **one** artifact:

```
Aviation.vst3\Contents\x86_64-win\Aviation.vst3   <- 64-bit FL (11.1+, 20, 21+, 25)
Aviation.vst3\Contents\x86-win\Aviation.vst3      <- FL 11 32-bit
```

Build both halves and merge with `scripts\build_windows_universal.bat`.
Install the same bundle to both VST3 paths; each FL loads the half matching itself.

Record the **exact** tester environment here before building `prototype-1-rc1` for Windows.

---

## Client environment (fill in)

| Item | Value | Recorded by | Date |
|------|-------|-------------|------|
| Windows version | _TBD — Windows 10 or 11, build number_ | | |
| CPU | _TBD — Intel/AMD x64_ | | |
| FL Studio 11 version | _TBD — e.g. 11.0.x vs 11.1+_ | | |
| FL 11 architecture | _TBD — 32-bit or 64-bit_ | | |
| FL Studio 20 version | _TBD — exact build_ | | |
| FL 20 architecture | _TBD — usually 64-bit_ | | |
| FL Studio 21+ version | _TBD — e.g. 21.x, 24.x_ | | |
| FL 21+ architecture | _TBD — 64-bit expected_ | | |
| FL Studio 25 version | _TBD — exact build (e.g. 25.2.3.4889)_ | | |
| FL 25 architecture | _TBD — 64-bit expected_ | | |
| Display scaling | _TBD — 100%, 125%, 150%, etc._ | | |
| Audio driver | _TBD — ASIO device/interface_ | | |

---

## Build matrix (derived from above)

| Host | Required VST3 build | Install path |
|------|---------------------|--------------|
| FL 11.1+ **64-bit** | Windows **x64** | `C:\Program Files\Common Files\VST3\` |
| FL 20 **64-bit** | Windows **x64** | same |
| FL 21+ **64-bit** | Windows **x64** | same |
| FL 25 **64-bit** (Windows) | Windows **x64** | same |
| FL 11 **32-bit** | Windows **x86** | `C:\Program Files (x86)\Common Files\VST3\` |

**64-bit rule:** All 64-bit FL hosts (11.1+, 20, 21, 24, 25, etc.) use the **same x64 VST3** at the standard 64-bit path. One Release x64 build covers the full 64-bit matrix.

**Universal rule:** ship the merged bundle to both paths. The client then does not
have to know their FL's bitness, and one ZIP serves every tester.

**Rules:**

- Do **not** use FL VST bridging for acceptance — native host/plugin bitness only.
- Build **x86** unconditionally for the universal bundle
  (`scripts\build_windows_universal.bat`). Drop it only via `x64only`, and only once
  FL 11 32-bit is confirmed absent — that decision must be recorded in the table above.
- Steinberg bundles may contain both `x86-win` and `x86_64-win` if both are built and tested.
- If Windows is 7/8 → **stop**; require Windows 10+ upgrade before ship.
- **FL 25 on Windows** is a valid Prototype #1 test host — use a **native empty project** created in FL 25 Windows, not an `.flp` saved on FL 25 macOS.
- **Project version rule:** A project saved in a **newer** FL cannot open in an **older** FL (e.g. FL 25 → FL 11 fails). Always create a fresh empty project per FL version under test.

---

## macOS tester environment (fill in)

| Item | Value | Recorded by | Date |
|------|-------|-------------|------|
| macOS version | _TBD_ | | |
| CPU | _TBD — Apple Silicon or Intel_ | | |
| Host(s) | _TBD — Logic / Live / Reaper / FL 25 macOS_ | | |
| Host architecture | _TBD — native arm64 or Rosetta_ | | |
| Display scaling | _TBD_ | | |

**Universal-binary rule:** the macOS `.pkg` ships an `arm64 + x86_64` universal VST3, so
it covers Apple Silicon (native and Rosetta hosts) and Intel without a second build.
Do **not** ship a single-arch build — a Rosetta-mode host on Apple Silicon loads the
`x86_64` slice, so an arm64-only binary silently fails to appear in some hosts.

**Deployment target:** macOS 10.15. Do not rely on the build machine's default.

---

## Blockers

**Windows**

- [ ] Client environment table complete (including any FL 21+ / FL 25 versions in scope)
- [ ] Build matrix confirmed (x64 vs x86)
- [ ] Windows 10+ confirmed (not Win7/8)

**macOS**

- [ ] macOS tester environment table complete
- [ ] Universal binary confirmed via `lipo -archs`
- [ ] Signing / notarization decision made (signed+notarized, or unsigned with a
      documented Gatekeeper workaround in `README_FIRST.txt`)

When the Windows blockers are checked, proceed with `scripts\build_windows.bat` on the
Windows build machine. The macOS artifact is produced on the dev Mac with
`./scripts/package_prototype_macos.sh` — from the same tagged commit.
