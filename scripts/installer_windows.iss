; =============================================================================
;  Aviation — Prototype 1 RC1 Windows installer (Inno Setup 6.3+)
;
;  Do not run this file directly. Build it with:
;      scripts\build_installer_windows.bat
;  which supplies the version/SHA defines below from git.
;
;  Defines expected from ISCC /D:
;      AppVersion   e.g. 0.1.0-rc1
;      GitSha       e.g. 0f04c959fd
;      Vst3Src      absolute path to the built Aviation.vst3 bundle folder
;      DocsSrc      absolute path to release\prototype1
;      OutDir       absolute output folder for the .exe
; =============================================================================

#ifndef AppVersion
  #define AppVersion "0.0.0-dev"
#endif
#ifndef GitSha
  #define GitSha "unknown"
#endif
#ifndef Vst3Src
  #error Vst3Src must be defined — run scripts\build_installer_windows.bat
#endif
#ifndef OutDir
  #define OutDir "..\dist"
#endif
#ifndef RcLabel
  #define RcLabel "RC1"
#endif
#ifndef SetupBase
  #define SetupBase "Aviation_Prototype1_" + RcLabel + "_Windows_Setup"
#endif

[Setup]
AppId={{A7E3C1D2-8B4F-4E6A-9C05-AV1AT10NRC01}
AppName=Aviation
AppVersion={#AppVersion}
AppVerName=Aviation Prototype 1 {#RcLabel} ({#AppVersion})
AppPublisher=Aviation
VersionInfoVersion=0.1.0
VersionInfoDescription=Aviation VST3 Instrument — Prototype 1 {#RcLabel}
VersionInfoTextVersion={#AppVersion} ({#GitSha})

; The VST3 install path is fixed by the VST3 spec — do not let the user choose it.
CreateAppDir=no
DisableDirPage=yes
DisableProgramGroupPage=yes

; Writing to Common Files requires elevation.
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

MinVersion=10.0
OutputDir={#OutDir}
OutputBaseFilename={#SetupBase}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=Aviation Prototype 1 {#RcLabel}
UninstallFilesDir={commoncf64}\VST3\Aviation-uninstall
LicenseFile=
InfoBeforeFile={#DocsSrc}\DOCS\KNOWN_ISSUES.txt

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "vst3x86"; \
  Description: "Also install for 32-bit FL Studio 11 (C:\Program Files (x86)\Common Files\VST3)"; \
  GroupDescription: "Additional install locations:"; \
  Flags: unchecked

[InstallDelete]
; Remove any previous Aviation build first so stale files cannot linger inside
; the bundle folder and get loaded instead of this RC.
Type: filesandordirs; Name: "{commoncf64}\VST3\Aviation.vst3"
Type: filesandordirs; Name: "{commoncf32}\VST3\Aviation.vst3"; Tasks: vst3x86

[Files]
Source: "{#Vst3Src}\*"; DestDir: "{commoncf64}\VST3\Aviation.vst3"; \
  Flags: ignoreversion recursesubdirs createallsubdirs

Source: "{#Vst3Src}\*"; DestDir: "{commoncf32}\VST3\Aviation.vst3"; \
  Flags: ignoreversion recursesubdirs createallsubdirs; Tasks: vst3x86

; Ship the tester-facing docs next to the plugin so they are findable after install.
Source: "{#DocsSrc}\DOCS\*.txt";     DestDir: "{commondocs}\Aviation Prototype 1\DOCS"; Flags: ignoreversion
Source: "{#DocsSrc}\TEST\*";         DestDir: "{commondocs}\Aviation Prototype 1\TEST"; Flags: ignoreversion
Source: "{#OutDir}\Aviation_Prototype1_{#RcLabel}_Windows\VERSION.txt"; \
  DestDir: "{commondocs}\Aviation Prototype 1"; Flags: ignoreversion skipifsourcedoesntexist

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Aviation.vst3"
Type: filesandordirs; Name: "{commoncf32}\VST3\Aviation.vst3"
Type: filesandordirs; Name: "{commondocs}\Aviation Prototype 1"

[Messages]
FinishedLabel=Aviation has been installed.%n%nNext: open FL Studio and rescan plugins%n(Options > Manage plugins > Find more plugins > Start scan),%nthen load Aviation on an instrument channel.%n%nConfirm the version in the plugin status bar matches VERSION.txt.
