# TaskbarHider

Lightweight Windows 11 utility that automatically hides the taskbar when no windows are open — nice for clean wallpaper screenshots or just idling.

- No windows visible → taskbar hides (hover the bottom edge to reveal)
- Any app window open → taskbar stays visible
- Multi-monitor support
- ~3 MB RAM, near-0% CPU (event-driven via Windows shell hooks)

## Install (recommended)

This release **does not ship a compiled `.exe`** — it ships the raw AutoHotkey v2 script and an installer. See [Why no `.exe`?](#why-no-exe) below for why.

1. Download the latest `TaskbarHider-vX.Y.Z.zip` from [Releases](https://github.com/babaJaan01/taskbarhider/releases).
2. Extract it anywhere.
3. Double-click **`install.bat`**.
   - If AutoHotkey v2 is missing, the installer will install it via `winget` automatically.
   - The script is copied to `%LOCALAPPDATA%\TaskbarHider\TaskbarHider.ahk`.
   - A Startup shortcut is created so it launches at login.
   - It starts running immediately.

That's it. The taskbar will start auto-hiding.

## Uninstall

Double-click **`uninstall.bat`** from the extracted folder (or re-download and run it). It will:

- Stop the running instance and restore the taskbar.
- Remove the Startup shortcut.
- Delete `%LOCALAPPDATA%\TaskbarHider`.

## Emergency: my taskbar is stuck hidden

- Press **`Ctrl+Alt+Shift+T`** anywhere — this force-exits TaskbarHider and restores the taskbar.
- Or double-click **`RestoreTaskbar.ahk`** from the repo.
- Or open Task Manager (`Ctrl+Shift+Esc`) → find `AutoHotkey64.exe` running `TaskbarHider.ahk` → End task. If the taskbar is still hidden, run `RestoreTaskbar.ahk`.

## Why no `.exe`?

The earlier v1.0 release shipped `TaskbarHider.exe` compiled via `Ahk2Exe`. Microsoft Defender started flagging it as malware (false positive). This is a well-known, long-running issue with **all** compiled AutoHotkey binaries — malware authors frequently abuse AutoHotkey to build droppers, so Defender's heuristics aggressively flag any `.exe` that looks like "AHK interpreter + embedded script + window enumeration + ShowWindow calls." It's not specific to this project.

Fixing it for good requires one of:

1. Distributing the raw `.ahk` + installer (what this release does). The official `AutoHotkey64.exe` interpreter is signed by AutoHotkey Foundation Limited and is already trusted by Defender.
2. Rewriting the tool as a native Win32 binary and code-signing it. **This repo is doing this too** — see the [`native/`](./native) folder for an in-progress native C++ port that will ship as a signed single-file `.exe`.

Distributing an unsigned Ahk2Exe build is not worth fighting — even if you submit a false-positive report and get it un-flagged, the next build gets re-flagged when Defender's model updates.

## Requirements

- Windows 10 (1809+) or Windows 11
- AutoHotkey v2 (installed automatically by `install.bat` if missing)

## How it works

- Registers a shell hook window (`RegisterShellHookWindow`) to receive `HSHELL_WINDOWCREATED` / `DESTROYED` / `ACTIVATED` events — event-driven, no polling for window changes.
- A 100 ms timer only runs hover-zone detection (needed because Windows does not send events for raw mouse motion near the screen edge).
- Walks `WinGetList()` and filters out system shell windows, tool windows, cloaked (UWP background) windows, etc. to decide whether any "real" user window is visible.
- Hides the taskbar via `ShowWindow(Shell_TrayWnd, SW_HIDE)` / restores it with `SW_SHOW`. Same for `Shell_SecondaryTrayWnd` on additional monitors.
- Handles `WM_ENDSESSION` so the taskbar is always restored on logoff/shutdown.

## Files

| File | Purpose |
| --- | --- |
| `TaskbarHider.ahk` | The main script |
| `RestoreTaskbar.ahk` | Emergency "force-show the taskbar" script |
| `install.ps1` / `install.bat` | Installer (adds Startup shortcut, installs AHK v2 via winget if needed) |
| `uninstall.ps1` / `uninstall.bat` | Uninstaller |
| `native/` | Work-in-progress native C++ port (tiny signed `.exe`, no AHK required) |

## License

MIT
