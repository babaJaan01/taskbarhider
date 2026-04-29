# Installer build notes

TaskbarHider uses Inno Setup for end-user installation.

The installer:
- installs to `%LocalAppData%\Programs\TaskbarHider`
- asks whether startup should be enabled (`Start TaskbarHider when I sign in`)
- launches TaskbarHider after install (unless silent mode)

## Build locally

1. Install Inno Setup 6.
2. Build native exe first (`native\build\bin\TaskbarHider.exe`).
3. Run ISCC from PowerShell:

```powershell
& "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" `
  "installer\TaskbarHider.iss" `
  "/DBuildArch=x64" `
  "/DSourceExePath=$((Resolve-Path 'native\\build\\bin\\TaskbarHider.exe').Path)" `
  "/O$(Resolve-Path '.').Path\\dist"
```

This generates `dist\TaskbarHider-Setup-x64.exe`.

For x86, build the 32-bit native exe and use `/DBuildArch=x86`.

## Startup behavior

- Checked: installer creates `%AppData%\Microsoft\Windows\Start Menu\Programs\Startup\TaskbarHider.lnk`
- Unchecked: no startup shortcut is created
- Installer closed/cancelled: no changes applied, user will be asked again next installer run
