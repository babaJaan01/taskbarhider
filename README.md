# TaskbarHider

Tiny Windows utility that hides the taskbar when your desktop is empty, then reveals it again when you open an app or move your mouse to the bottom edge.

- No visible windows -> taskbar hides
- Hover the bottom edge -> taskbar reveals
- Open any normal app window -> taskbar stays visible
- Multi-monitor support
- Native Win32 build: about 100 KB on disk, about 1 MB RAM, near-0% CPU at idle

## Download

Get the latest files from [Releases](https://github.com/babaJaan01/taskbarhider/releases).

### Recommended: native build

- Download `TaskbarHider-x64.exe` on almost every modern PC.
- Use `TaskbarHider-x86.exe` only on 32-bit Windows.
- Double-click the exe to run it.
- Right-click the tray icon to restore the taskbar or exit.
- Emergency exit hotkey: `Ctrl+Alt+Shift+T`

### Fallback: AutoHotkey build

If you prefer a script-based install, download `TaskbarHider-vX.Y.Z-ahk.zip`, extract it, and run `install.bat`.

That installer:
- installs AutoHotkey v2 with `winget` if needed
- copies the script to `%LOCALAPPDATA%\TaskbarHider`
- creates a Startup shortcut
- launches it immediately

## x64 vs x86

- `x64` = 64-bit Windows. This is the right choice for almost everyone.
- `x86` = 32-bit Windows. Only use this if your OS is actually 32-bit.

If you are unsure, use `x64`.

## SmartScreen and Defender

The native build is a normal Win32 executable and should not trigger Microsoft Defender malware detection.

Windows SmartScreen may still show an "Unknown publisher" warning on first run because unsigned apps start with no reputation. That is expected for small new utilities and is separate from Defender malware flagging. Code signing fixes that later.

## Why there are two builds

Earlier versions shipped a compiled AutoHotkey exe built with `Ahk2Exe`. Microsoft Defender started flagging that exe as malware, which is a common false-positive pattern for compiled AutoHotkey apps.

This repo now ships two safer options:

- a native C++ Win32 build, which is the main product
- a raw AutoHotkey v2 script plus installer, which avoids the Defender problem because it runs through the signed official AutoHotkey interpreter

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
- or run `RestoreTaskbar.ahk`
- or end `TaskbarHider.exe` / `AutoHotkey64.exe` in Task Manager and then run `RestoreTaskbar.ahk`

## Repo layout

- `native/` - native Win32 source and build files
- `TaskbarHider.ahk` - AutoHotkey fallback version
- `RestoreTaskbar.ahk` - emergency taskbar restorer
- `install.*` / `uninstall.*` - installer helpers for the AHK version
- `.github/workflows/native-release.yml` - CI and release automation

## Build from source

See `native/README.md` for local native build instructions.

## License

MIT
