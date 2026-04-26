# TaskbarHider — native port

A native Win32 C++ rewrite of the AutoHotkey script. Same behavior, much smaller
and faster binary, no AutoHotkey dependency, and no Ahk2Exe false-positive
problem.

- **Binary size**: ~80 KB (static CRT, no redistributable needed).
- **RAM**: ~1 MB.
- **Dependencies**: none. Ships a single `.exe`.
- **Supports**: Windows 10 (1809+) and Windows 11, x64 and x86.

## Building locally

Prerequisites:
- Visual Studio 2022 with "Desktop development with C++" workload
  (or just the Build Tools + Windows SDK), OR
- Any recent MSVC + CMake (>= 3.20)

From a **"x64 Native Tools Command Prompt for VS 2022"** (or a regular shell
with CMake on `PATH`):

```bat
cd native
cmake -S . -B build -A x64
cmake --build build --config Release
```

Output: `native\build\bin\TaskbarHider.exe`

To build a 32-bit version: `-A Win32` instead of `-A x64`.

## Running

Just double-click `TaskbarHider.exe`. Right-click the tray icon for the exit
menu. `Ctrl+Alt+Shift+T` is a global emergency-exit hotkey that always
restores the taskbar.

For auto-start: press `Win+R`, type `shell:startup`, and drop a shortcut to
`TaskbarHider.exe` in the folder that opens.

## Why this exists

The compiled AutoHotkey version (`Ahk2Exe` output) is flagged by Microsoft
Defender as malware. This is a false positive but it's a well-known category
with no fix other than code-signing with built-up reputation (expensive and
doesn't fully solve it). The native binary side-steps the AHK heuristic
entirely — it's just a normal small Win32 program.

## Signing & distribution plan

The GitHub Actions workflow (`.github/workflows/native-release.yml`) builds
the exe on every tag push. To avoid SmartScreen "Unknown publisher" prompts,
sign the artifact before uploading to the GitHub release. Cheapest options:

- **[Azure Trusted Signing](https://learn.microsoft.com/en-us/azure/trusted-signing/)** (~$10/month) — EV-equivalent reputation, integrates with
  `azure/trusted-signing-action` in GitHub Actions. **Recommended.**
- SSL.com / Sectigo OV code-signing cert (~$60–100/year) — OV, slower
  SmartScreen warmup, but cheapest one-time setup.
- For unsigned personal use: SmartScreen will warn once — click "More info"
  → "Run anyway". Defender won't flag the native build in the first place.

Once signed, submit to [winget-pkgs](https://github.com/microsoft/winget-pkgs)
so users can `winget install babaJaan01.TaskbarHider`. Winget-delivered
binaries inherit Microsoft's trust.

## Architecture

Single-file `src/main.cpp`. Roughly:

- `wWinMain` — registers window class, acquires single-instance mutex, pumps messages.
- `WndProc` — handles:
  - `WM_CREATE`: register shell hook, register `Ctrl+Alt+Shift+T` hotkey, start timers, add tray icon, do initial update.
  - `SHELLHOOK`: forward to `OnShellEvent` which debounces via a one-shot 30 ms timer.
  - `WM_TIMER`: `kTimerIdMain` (100 ms, hover detection + safety poll), `kTimerIdRefresh` (30 s, re-cache taskbar handles).
  - `WM_QUERYENDSESSION` / `WM_ENDSESSION`: restore taskbar so the user is never stranded on logoff/shutdown.
  - `WM_DPICHANGED` / `WM_DISPLAYCHANGE`: re-cache taskbar handles (handles monitor changes, resolution changes).
  - `kTrayMsg` + `WM_COMMAND`: tray menu (Restore, About, Exit).
  - `WM_DESTROY`: cleanup, always `ShowTaskbar()` before exit.
- `HasVisibleWindows`: `EnumWindows` filtering out system shell classes, tool windows, cloaked (UWP background) windows, and empty titles.
- `IsAnyFlyoutOpen`: checks for open Start/Search/Action Center/tray-overflow windows so the taskbar stays visible while the user interacts with them.
- `IsMouseInTaskbarZone`: checks bottom 5 px of every monitor (per `EnumDisplayMonitors`), the taskbar rects themselves, and any tray flyout under the cursor.

## Tests / verification

For now this is manual:

1. Build Release, run `TaskbarHider.exe`.
2. Confirm taskbar hides when desktop is clean.
3. Open Notepad → taskbar comes back.
4. Close Notepad → taskbar hides again.
5. Hover bottom edge → taskbar reveals. Move away → it hides.
6. Click tray icon → menu works. "Exit" restores taskbar and quits.
7. Press `Ctrl+Alt+Shift+T` from another app → same as Exit.
8. Plug/unplug an external monitor → secondary taskbar handled too.
