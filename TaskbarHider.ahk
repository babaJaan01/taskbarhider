#Requires AutoHotkey v2.0
#SingleInstance Force
Persistent
#NoTrayIcon ; Remove tray icon for minimal footprint (optional - remove this line if you want tray icon)

; ============================================
; TaskbarHider - Smart Taskbar Auto-Hide
; GitHub: https://github.com/babaJaan01/taskbarhider.git
; License: MIT
; 
; Behavior:
;   - No windows visible → Taskbar hides (hover to reveal)
;   - Windows open → Taskbar always visible
; ============================================

; State
global gTaskbarHidden := false
global gLastWindowState := true
global gHoverRevealed := false

; Cache taskbar handles (refreshed periodically)
global gMainTaskbar := 0
global gSecondaryTaskbars := []

; Register shell hooks (event-driven)
gShellHookMsg := DllCall("RegisterWindowMessage", "Str", "SHELLHOOK", "UInt")
DllCall("RegisterShellHookWindow", "Ptr", A_ScriptHwnd)
OnMessage(gShellHookMsg, OnShellMessage)

; Main timer for hover detection + safety checks
SetTimer(MainLoop, 100)

; Refresh taskbar handles periodically (handles monitor changes)
RefreshTaskbarHandles()
SetTimer(RefreshTaskbarHandles, 30000)

; Initial state check
UpdateTaskbar()

; Cleanup on exit
OnExit((*) => Cleanup())

; ============================================
; CLEANUP
; ============================================
Cleanup() {
    SetTimer(MainLoop, 0)
    SetTimer(RefreshTaskbarHandles, 0)
    DllCall("DeregisterShellHookWindow", "Ptr", A_ScriptHwnd)
    ShowTaskbar()
}

; ============================================
; REFRESH TASKBAR HANDLES
; ============================================
RefreshTaskbarHandles() {
    global gMainTaskbar, gSecondaryTaskbars
    
    gMainTaskbar := DllCall("FindWindow", "Str", "Shell_TrayWnd", "Ptr", 0, "Ptr")
    gSecondaryTaskbars := []
    
    hwnd := 0
    Loop {
        hwnd := DllCall("FindWindowEx", "Ptr", 0, "Ptr", hwnd, "Str", "Shell_SecondaryTrayWnd", "Ptr", 0, "Ptr")
        if !hwnd
            break
        gSecondaryTaskbars.Push(hwnd)
    }
}

; ============================================
; SHELL MESSAGE HANDLER (Event-driven)
; ============================================
OnShellMessage(wParam, lParam, *) {
    static events := Map(1, 1, 2, 1, 4, 1, 6, 1, 0x8004, 1) ; CREATED, DESTROYED, ACTIVATED, REDRAW, RUDEAPP
    if events.Has(wParam)
        SetTimer(UpdateTaskbar, -30)
}

; ============================================
; MAIN LOOP
; ============================================
MainLoop() {
    global gTaskbarHidden, gHoverRevealed, gLastWindowState
    
    ; Safety check for missed window events
    currentState := HasVisibleWindows()
    if (currentState != gLastWindowState) {
        gLastWindowState := currentState
        UpdateTaskbar()
        return
    }
    
    ; Hover detection (only in hidden mode)
    if (!gTaskbarHidden)
        return
    
    mouseInZone := IsMouseInTaskbarZone()
    
    if (mouseInZone && !gHoverRevealed) {
        ShowTaskbar()
        gHoverRevealed := true
    } else if (!mouseInZone && gHoverRevealed) {
        HideTaskbar()
        gHoverRevealed := false
    }
}

; ============================================
; CHECK IF MOUSE IS IN TASKBAR ZONE
; (Bottom edge OR on taskbar OR on flyout)
; ============================================
IsMouseInTaskbarZone() {
    static HOVER_ZONE := 5
    
    CoordMode("Mouse", "Screen")
    MouseGetPos(&mx, &my)
    
    ; Check bottom edge of any monitor
    monCount := MonitorGetCount()
    Loop monCount {
        MonitorGet(A_Index, &mL, &mT, &mR, &mB)
        if (mx >= mL && mx < mR && my >= mB - HOVER_ZONE)
            return true
    }
    
    ; Check if on taskbar
    if IsPointOnTaskbar(mx, my)
        return true
    
    ; Check if on flyout
    if IsPointOnFlyout(mx, my)
        return true
    
    return false
}

; ============================================
; CHECK IF POINT IS ON TASKBAR
; ============================================
IsPointOnTaskbar(x, y) {
    global gMainTaskbar, gSecondaryTaskbars
    
    if gMainTaskbar && PointInWindow(gMainTaskbar, x, y)
        return true
    
    for hwnd in gSecondaryTaskbars
        if PointInWindow(hwnd, x, y)
            return true
    
    return false
}

; ============================================
; CHECK IF POINT IS ON FLYOUT
; ============================================
IsPointOnFlyout(x, y) {
    hwnd := DllCall("WindowFromPoint", "Int64", (y << 32) | (x & 0xFFFFFFFF), "Ptr")
    if !hwnd
        return false
    
    hwnd := DllCall("GetAncestor", "Ptr", hwnd, "UInt", 2, "Ptr")
    if !hwnd
        return false
    
    try {
        className := WinGetClass(hwnd)
        static flyouts := "NotifyIconOverflowWindow,TopLevelWindowForOverflowXamlIsland,Windows.UI.Core.CoreWindow,XamlExplorerHostIslandWindow,Shell_InputSwitchTopLevelWindow,TaskListThumbnailWnd,ControlCenterWindow"
        
        if InStr(flyouts, className)
            return true
        
        ; Check if owned by taskbar
        owner := DllCall("GetWindow", "Ptr", hwnd, "UInt", 4, "Ptr")
        if owner {
            ownerClass := WinGetClass(owner)
            if (ownerClass = "Shell_TrayWnd" || ownerClass = "Shell_SecondaryTrayWnd")
                return true
        }
    }
    return false
}

; ============================================
; POINT IN WINDOW CHECK
; ============================================
PointInWindow(hwnd, x, y) {
    static rect := Buffer(16, 0)
    if !DllCall("GetWindowRect", "Ptr", hwnd, "Ptr", rect)
        return false
    return x >= NumGet(rect, 0, "Int") && x < NumGet(rect, 8, "Int") 
        && y >= NumGet(rect, 4, "Int") && y < NumGet(rect, 12, "Int")
}

; ============================================
; UPDATE TASKBAR STATE
; ============================================
UpdateTaskbar() {
    global gTaskbarHidden, gLastWindowState, gHoverRevealed
    hasWindows := HasVisibleWindows()
    gLastWindowState := hasWindows
    
    if (hasWindows) {
        gTaskbarHidden := false
        gHoverRevealed := false
        ShowTaskbar()
    } else {
        gTaskbarHidden := true
        gHoverRevealed := false
        HideTaskbar()
    }
}

; ============================================
; CHECK FOR VISIBLE WINDOWS
; ============================================
HasVisibleWindows() {
    if IsAnyFlyoutOpen()
        return true
    
    static sysClasses := "Progman,WorkerW,Shell_TrayWnd,Shell_SecondaryTrayWnd,Windows.UI.Core.CoreWindow,ApplicationFrameWindow,TopLevelWindowForOverflowXamlIsland,Shell_InputSwitchTopLevelWindow,XamlExplorerHostIslandWindow,InputApp,LockApp,Windows.UI.Composition.DesktopWindowContentBridge,EdgeUiInputTopWndClass,NarratorHelperWindow,Shell_LightDismissOverlay,SearchHost,NotifyIconOverflowWindow"
    
    for hwnd in WinGetList() {
        try {
            if WinGetMinMax(hwnd) = -1
                continue
            
            className := WinGetClass(hwnd)
            if InStr(sysClasses, className)
                continue
            
            title := WinGetTitle(hwnd)
            if (title = "" || title = "Program Manager" || title = "Windows Shell Experience Host")
                continue
            
            if (WinGetExStyle(hwnd) & 0x80) ; WS_EX_TOOLWINDOW
                continue
            
            if IsWindowCloaked(hwnd)
                continue
            
            return true
        }
    }
    return false
}

; ============================================
; CHECK IF ANY FLYOUT IS OPEN
; ============================================
IsAnyFlyoutOpen() {
    static flyoutClasses := ["NotifyIconOverflowWindow", "TopLevelWindowForOverflowXamlIsland", 
                             "Shell_InputSwitchTopLevelWindow", "TaskListThumbnailWnd", "ControlCenterWindow"]
    
    for cls in flyoutClasses
        if WinExist("ahk_class " . cls)
            return true
    
    for hwnd in WinGetList("ahk_class Windows.UI.Core.CoreWindow") {
        try {
            if !IsWindowCloaked(hwnd) {
                title := WinGetTitle(hwnd)
                if (title != "Cortana" && title != "Search")
                    return true
            }
        }
    }
    return false
}

; ============================================
; CHECK IF WINDOW IS CLOAKED
; ============================================
IsWindowCloaked(hwnd) {
    cloaked := 0
    DllCall("dwmapi\DwmGetWindowAttribute", "Ptr", hwnd, "Int", 14, "Int*", &cloaked, "Int", 4)
    return cloaked
}

; ============================================
; SHOW/HIDE TASKBAR
; ============================================
ShowTaskbar() {
    global gMainTaskbar, gSecondaryTaskbars
    if gMainTaskbar
        DllCall("ShowWindow", "Ptr", gMainTaskbar, "Int", 5)
    for hwnd in gSecondaryTaskbars
        DllCall("ShowWindow", "Ptr", hwnd, "Int", 5)
}

HideTaskbar() {
    global gMainTaskbar, gSecondaryTaskbars
    if gMainTaskbar
        DllCall("ShowWindow", "Ptr", gMainTaskbar, "Int", 0)
    for hwnd in gSecondaryTaskbars
        DllCall("ShowWindow", "Ptr", hwnd, "Int", 0)
}
