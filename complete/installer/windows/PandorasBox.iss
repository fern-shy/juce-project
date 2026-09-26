; Compiled by scripts/release_windows.ps1, which defines AppVersion, Vst3Dir,
; DocsDir, OutputDir, and OutputBaseFilename.

#ifndef AppVersion
  #error AppVersion must be defined
#endif

[Setup]
; Keep AppId unchanged forever so new versions upgrade existing installs.
AppId={{BDBCA834-D4A1-4683-A5A1-8F2ADE74B641}
AppName=Pandoras Box
AppVersion={#AppVersion}
AppVerName=Pandoras Box {#AppVersion}
AppPublisher=FernShy
AppPublisherURL=https://fernshy.com
AppSupportURL=https://fernshy.com
AppCopyright=Copyright (c) 2026 FernShy
VersionInfoVersion={#AppVersion}
DefaultDirName={commonpf64}\FernShy\Pandoras Box
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableWelcomePage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
OutputDir={#OutputDir}
OutputBaseFilename={#OutputBaseFilename}
SetupIconFile={#Vst3Dir}\Plugin.ico
UninstallDisplayIcon={app}\PandorasBox.ico
UninstallDisplayName=Pandoras Box (VST3)
WizardStyle=modern
Compression=lzma2
SolidCompression=yes

[InstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Pandoras Box.vst3"
; Folders left behind by earlier ZIP releases dragged whole into the VST3 folder.
Type: filesandordirs; Name: "{commoncf64}\VST3\PandorasBox-*-Windows-x64*"

[Dirs]
Name: "{commoncf64}\VST3\Pandoras Box.vst3"; Attribs: system

[Files]
Source: "{#Vst3Dir}\*"; DestDir: "{commoncf64}\VST3\Pandoras Box.vst3"; Excludes: "desktop.ini"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Vst3Dir}\desktop.ini"; DestDir: "{commoncf64}\VST3\Pandoras Box.vst3"; Attribs: hidden system; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#Vst3Dir}\Plugin.ico"; DestDir: "{app}"; DestName: "PandorasBox.ico"; Flags: ignoreversion
Source: "{#DocsDir}\README.txt"; DestDir: "{app}"; Flags: ignoreversion isreadme
Source: "{#DocsDir}\LICENSE.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#DocsDir}\THIRD_PARTY_LICENSES.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#DocsDir}\INTER_FONT_LICENSE.md"; DestDir: "{app}"; Flags: ignoreversion

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Pandoras Box.vst3"

[Messages]
FinishedHeadingLabel=Pandoras Box is installed
FinishedLabel=Pandoras Box was installed to C:\Program Files\Common Files\VST3.%n%nOpen FL Studio, Ableton Live, or any other VST3 host and rescan your plugins. Pandoras Box appears as an effect by FernShy.
