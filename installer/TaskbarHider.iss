#define MyAppName "TaskbarHider"
#define MyAppVersion "2.0"
#define MyAppPublisher "Shayaan Tanveer"
#define MyAppURL "https://github.com/babaJaan01/taskbarhider"

#ifndef BuildArch
  #define BuildArch "x64"
#endif

#ifndef SourceExePath
  #error SourceExePath must be provided to ISCC.
#endif

[Setup]
AppId={{8A0E3008-4BA0-4D44-A25C-E68CF6A6022B}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={localappdata}\Programs\TaskbarHider
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
DisableDirPage=no
PrivilegesRequired=lowest
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={app}\TaskbarHider.exe
OutputDir=.
OutputBaseFilename=TaskbarHider-Setup-{#BuildArch}
Compression=lzma
SolidCompression=yes
WizardStyle=modern
SetupLogging=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "startup"; Description: "Start TaskbarHider when I sign in"; Flags: unchecked

[Files]
Source: "{#SourceExePath}"; DestDir: "{app}"; DestName: "TaskbarHider.exe"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\TaskbarHider"; Filename: "{app}\TaskbarHider.exe"
Name: "{autostartup}\TaskbarHider"; Filename: "{app}\TaskbarHider.exe"; Tasks: startup

[Run]
Filename: "{app}\TaskbarHider.exe"; Description: "Launch TaskbarHider now"; Flags: nowait postinstall skipifsilent
