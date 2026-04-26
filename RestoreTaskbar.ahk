#Requires AutoHotkey v2.0

; Emergency taskbar restorer. Double-click this file (or run with AutoHotkey64.exe)
; if the taskbar is stuck hidden — for example after a TaskbarHider crash.
;
; No MsgBox is shown by default so it can be used non-interactively.
; Pass /quiet to suppress all output (e.g. from scripts).

SW_SHOW := 5
silent  := false
for arg in A_Args
    if (arg = "/quiet" || arg = "--quiet")
        silent := true

taskbar := DllCall("FindWindow", "Str", "Shell_TrayWnd", "Ptr", 0, "Ptr")
if taskbar
    DllCall("ShowWindow", "Ptr", taskbar, "Int", SW_SHOW)

secondary := 0
Loop {
    secondary := DllCall("FindWindowEx", "Ptr", 0, "Ptr", secondary,
                         "Str", "Shell_SecondaryTrayWnd", "Ptr", 0, "Ptr")
    if !secondary
        break
    DllCall("ShowWindow", "Ptr", secondary, "Int", SW_SHOW)
}

if !silent
    TrayTip("Taskbar restored", "TaskbarHider", 0x10)

ExitApp
