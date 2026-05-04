# TaskbarHider - native build

Native Win32 C++ version of TaskbarHider. This is now the only supported build
of the project.

- Binary size: about 100 KB
- Runtime memory: about 1 MB
- Idle CPU: effectively 0%
- Supports: Windows 10 (1809+) and Windows 11
- Targets: x64 and x86

## Building locally

Prerequisites:
- Visual Studio 2022 with "Desktop development with C++" workload
  (or just the Build Tools + Windows SDK)
- Rust stable with the matching MSVC target (`x86_64-pc-windows-msvc` or
  `i686-pc-windows-msvc`)
- CMake >= 3.20

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

Run `TaskbarHider.exe`, then use the tray icon for Restore / About / Exit.
`Ctrl+Alt+Shift+T` is a global emergency-exit hotkey that always restores the
taskbar.

For auto-start, set `start_on_startup = true` in `taskbarhider.toml`. The app
reads config only when it starts and syncs the Startup-folder shortcut then.

For end users, the recommended path is the setup wizard release artifact
(`TaskbarHider-Setup-x64.exe` / `TaskbarHider-Setup-x86.exe`) which can
configure startup and app-attention behavior during install.

## Why this exists

The old compiled AutoHotkey exe was being flagged by Microsoft Defender, which
is a common false positive for `Ahk2Exe` output. The native build avoids that
heuristic entirely and gives a cleaner public release story.

## Signing and distribution

The GitHub Actions workflow in `.github/workflows/native-release.yml` builds
the exe on every push and publishes release assets on `v*` tags.

To avoid SmartScreen "Unknown publisher" prompts, sign the binary before
shipping it. Practical options:

- **[Azure Trusted Signing](https://learn.microsoft.com/en-us/azure/trusted-signing/)** (~$10/month) — EV-equivalent reputation, integrates with
  `azure/trusted-signing-action` in GitHub Actions. **Recommended.**
- SSL.com / Sectigo OV code-signing cert (~$60–100/year) — OV, slower
  SmartScreen warmup, but cheapest one-time setup.
- For unsigned personal use: SmartScreen will warn once - click "More info"
  → "Run anyway". Defender won't flag the native build in the first place.

Once signed, submit to [winget-pkgs](https://github.com/microsoft/winget-pkgs)
so users can `winget install babaJaan01.TaskbarHider`. Winget-delivered
binaries inherit Microsoft's trust.

## Architecture

The Win32 host lives in `src/main.cpp`; the taskbar state machine and rule
engine live in the Rust static library under `rust_core/`.

- `wWinMain` sets up the hidden window, single-instance mutex, and message loop
- `WndProc` handles shell events, timers, the tray icon, hotkeys, and cleanup
- `HasVisibleWindows()` decides whether the desktop is "empty"
- `IsAnyFlyoutOpen()` keeps the taskbar visible while tray/Start/search UI is open
- `IsMouseInTaskbarZone()` handles hover-to-reveal
- Rust core decides show/hide actions, hide-delay scheduling, and app-attention behavior
- `ShowTaskbar()` / `HideTaskbar()` update the main and secondary taskbars

## Manual verification

1. Build Release, run `TaskbarHider.exe`.
2. Confirm taskbar hides when desktop is clean.
3. Open Notepad → taskbar comes back.
4. Close Notepad → taskbar hides again.
5. Hover bottom edge -> taskbar reveals. Move away -> it hides.
6. Click tray icon → menu works. "Exit" restores taskbar and quits.
7. Press `Ctrl+Alt+Shift+T` from another app → same as Exit.
8. Plug/unplug an external monitor → secondary taskbar handled too.
9. Toggle `start_on_startup` in `taskbarhider.toml`, restart TaskbarHider, and verify the Startup-folder shortcut updates.
10. Toggle `show_on_app_attention` in `taskbarhider.toml` and verify app-attention behavior.
