# TaskbarHider

Tiny Windows utility that hides the taskbar when your desktop is empty, then reveals it again when you open an app or move your mouse to the bottom edge.

- No visible windows -> taskbar hides
- Hover the bottom edge -> taskbar reveals
- Open any normal app window -> taskbar stays visible
- Multi-monitor support
- Native Win32 build: about 100 KB on disk, about 1 MB RAM, near-0% CPU at idle

## Download

Get the latest files from [Releases](https://github.com/babaJaan01/taskbarhider/releases).

### Installer wizard (only supported download)

- Download `TaskbarHider-Setup-x64.exe` on almost every modern PC.
- Use `TaskbarHider-Setup-x86.exe` only on 32-bit Windows.
- The setup wizard asks whether you want TaskbarHider to start when you sign in.
- If you cancel setup, no changes are applied and the wizard will ask again next time you run it.

## Install

1. Download and run the setup file from Releases:
   - `TaskbarHider-Setup-x64.exe` (recommended)
   - `TaskbarHider-Setup-x86.exe` (32-bit Windows only)
2. Choose install folder (default is `%LocalAppData%\\Programs\\TaskbarHider`).
3. Choose whether TaskbarHider should start when you sign in.
4. Finish setup and launch.

## Uninstall

- Open **Settings -> Apps -> Installed apps**
- Find **TaskbarHider**
- Click **Uninstall**

This removes the app and startup shortcut created by the installer.

## Verify download integrity (SHA256SUMS)

Each release includes a `SHA256SUMS.txt` file. It contains trusted SHA-256
hashes for the installer files.

Why this matters:
- proves the file was not corrupted during download
- helps detect tampering
- lets you verify you got the exact file published in the release

How to verify on Windows PowerShell:

1. Download `SHA256SUMS.txt` and your installer (`TaskbarHider-Setup-x64.exe`).
2. In the folder with those files, run:

```powershell
Get-FileHash .\TaskbarHider-Setup-x64.exe -Algorithm SHA256
```

3. Compare the `Hash` output with the matching line in `SHA256SUMS.txt`.
   They must be identical.

Can setup verify this automatically right now?
- Not currently. Inno Setup does not verify against your release checksum file
  by default before launch.
- For now, checksum verification is a manual pre-install safety check.

## x64 vs x86

- `x64` = 64-bit Windows. This is the right choice for almost everyone.
- `x86` = 32-bit Windows. Only use this if your OS is actually 32-bit.

If you are unsure, use `x64`.

## SmartScreen and Defender

The native build is a normal Win32 executable and should not trigger Microsoft Defender malware detection.

Windows SmartScreen may still show an "Unknown publisher" warning on first run because unsigned apps start with no reputation. That is expected for small new utilities and is separate from Defender malware flagging. Code signing fixes that later.

## Why this is native-only

Earlier versions shipped a compiled AutoHotkey exe built with `Ahk2Exe`. Microsoft Defender started flagging that exe as malware, which is a common false-positive pattern for compiled AutoHotkey apps.

This repo now ships only the native C++ Win32 build. That keeps downloads simpler, avoids the AutoHotkey false-positive problem, and gives users a straightforward single-exe install path.

## Safety and privacy

TaskbarHider does not:

- send network traffic
- collect telemetry
- download code
- touch credentials
- inject into other processes
- require administrator privileges

It only watches normal shell/window state and shows or hides the Windows taskbar.

## Emergency recovery

If the taskbar ever gets stuck hidden:

- press `Ctrl+Alt+Shift+T`
- or end `TaskbarHider.exe` in Task Manager

## Repo layout

- `native/` - native Win32 source and build files
- `installer/` - Inno Setup script and installer docs
- `.github/workflows/native-release.yml` - CI and release automation

## Build from source

See `native/README.md` for local native build instructions.

## License

MIT
