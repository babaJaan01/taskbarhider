## TaskbarHider v1.0

## I don't know why but Windows Defender Antivirus is marking it as a virus. Will see

Lightweight windows 11 utility that automatically hides the taskbar when no windows are open. nice for screenshots of your wallpaper or just idling.

Built with [AutoHotkey v2](https://github.com/AutoHotkey/AutoHotkey) (and cursor), so a more "efficient" way to do this is definitely possible with WinAPI and C++. 

### Features
-  **Ultra-lightweight** - ~3MB RAM, 0% CPU
-  **Smart auto-hide** - Taskbar hides when desktop is empty
-  **Hover to reveal** - Move mouse to bottom edge to show taskbar
-  **Multi-monitor support** - Should work with multiple monitors

### Installation
1. Download `TaskbarHider.exe` from releases
2. Run it
3. Optional: Add to startup folder (`Win+R` → `shell:startup`)

### Requirements
- Windows 10/11
- No dependencies required, unless you modify the .ahk files. In that case you need Autohotkey v2.

### Issues?
- go to task manager and search for `TaskbarHider.exe` and end the task.
