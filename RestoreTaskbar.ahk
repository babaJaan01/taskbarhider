#Requires AutoHotkey v2.0

; Quick script to restore hidden taskbar

SW_SHOW := 5

; Show main taskbar
taskbar := DllCall("FindWindow", "Str", "Shell_TrayWnd", "Ptr", 0, "Ptr")
if taskbar
    DllCall("ShowWindow", "Ptr", taskbar, "Int", SW_SHOW)

; Show secondary taskbars (multi-monitor)
secondaryTaskbar := 0
Loop {
    secondaryTaskbar := DllCall("FindWindowEx", "Ptr", 0, "Ptr", secondaryTaskbar, 
                                 "Str", "Shell_SecondaryTrayWnd", "Ptr", 0, "Ptr")
    if !secondaryTaskbar
        break
    DllCall("ShowWindow", "Ptr", secondaryTaskbar, "Int", SW_SHOW)
}

MsgBox("Taskbar restored!", "Success")
ExitApp
