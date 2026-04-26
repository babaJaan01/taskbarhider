// TaskbarHider — native Win32 port.
//
// Behavior (matches the original AutoHotkey v2 script):
//   * No user windows visible  -> hide Shell_TrayWnd + Shell_SecondaryTrayWnd.
//   * Mouse at the bottom edge  -> reveal the taskbar while hovering.
//   * Any real user window open -> taskbar always shown.
//
// Why native? The AHK-compiled build was being flagged by Microsoft Defender
// (well-known false-positive category). A small, signed, native binary avoids
// the AHK heuristic entirely and has near-zero overhead.

#include <windows.h>
#include <shellapi.h>
#include <dwmapi.h>
#include <string>

#include "resource.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "dwmapi.lib")

namespace {

// --- constants ---------------------------------------------------------------

constexpr wchar_t kWindowClass[]  = L"TaskbarHider_Hidden_Window_v1";
constexpr wchar_t kWindowTitle[]  = L"TaskbarHider";
constexpr wchar_t kMutexName[]    = L"Local\\TaskbarHider_SingleInstance_v1";

constexpr UINT  kTrayMsg             = WM_APP + 1;
constexpr UINT  kShellHookUpdateMsg  = WM_APP + 2;
constexpr UINT_PTR kTimerIdMain      = 1;
constexpr UINT_PTR kTimerIdRefresh   = 2;
constexpr UINT_PTR kTimerIdDebounce  = 3;
constexpr UINT  kMainTimerMs         = 100;
constexpr UINT  kRefreshTimerMs      = 30000;
constexpr UINT  kDebounceMs          = 30;
constexpr int   kHoverZonePx         = 5;

constexpr int   kEmergencyHotkeyId   = 1;

// --- globals -----------------------------------------------------------------

HINSTANCE    g_hInstance        = nullptr;
HWND         g_hWnd             = nullptr;
UINT         g_shellHookMsg     = 0;
bool         g_shellHookRegistered = false;

HWND         g_mainTaskbar      = nullptr;
// Up to N secondary taskbars (one per additional monitor).
constexpr size_t kMaxSecondary = 16;
HWND         g_secondaryTaskbars[kMaxSecondary] = {};
size_t       g_secondaryCount   = 0;

bool         g_taskbarHidden    = false;
bool         g_lastWindowState  = true;
bool         g_hoverRevealed    = false;

NOTIFYICONDATAW g_nid            = {};
bool            g_trayAdded      = false;

// --- forward decls -----------------------------------------------------------

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void RefreshTaskbarHandles();
void UpdateTaskbar();
bool HasVisibleWindows();
bool IsAnyFlyoutOpen();
bool IsMouseInTaskbarZone();
bool IsPointOnTaskbar(POINT);
bool IsPointOnFlyout(POINT);
bool IsWindowCloaked(HWND);
void HideTaskbar();
void ShowTaskbar();
void MainLoop();
void OnShellEvent(WPARAM wParam);
void AddTrayIcon();
void RemoveTrayIcon();
void ShowTrayMenu();

// --- utilities ---------------------------------------------------------------

bool ClassNameMatchesAny(HWND hwnd, std::initializer_list<const wchar_t*> names) {
    wchar_t buf[128] = {};
    int n = GetClassNameW(hwnd, buf, _countof(buf));
    if (n <= 0) return false;
    for (const wchar_t* name : names) {
        if (wcscmp(buf, name) == 0) return true;
    }
    return false;
}

bool ClassNameEquals(HWND hwnd, const wchar_t* name) {
    wchar_t buf[128] = {};
    int n = GetClassNameW(hwnd, buf, _countof(buf));
    if (n <= 0) return false;
    return wcscmp(buf, name) == 0;
}

// --- taskbar handle cache ---------------------------------------------------

void RefreshTaskbarHandles() {
    g_mainTaskbar = FindWindowW(L"Shell_TrayWnd", nullptr);
    g_secondaryCount = 0;
    HWND next = nullptr;
    while ((next = FindWindowExW(nullptr, next, L"Shell_SecondaryTrayWnd", nullptr)) != nullptr) {
        if (g_secondaryCount >= kMaxSecondary) break;
        g_secondaryTaskbars[g_secondaryCount++] = next;
    }
}

void ShowTaskbar() {
    if (g_mainTaskbar) ShowWindow(g_mainTaskbar, SW_SHOW);
    for (size_t i = 0; i < g_secondaryCount; ++i)
        if (g_secondaryTaskbars[i]) ShowWindow(g_secondaryTaskbars[i], SW_SHOW);
}

void HideTaskbar() {
    if (g_mainTaskbar) ShowWindow(g_mainTaskbar, SW_HIDE);
    for (size_t i = 0; i < g_secondaryCount; ++i)
        if (g_secondaryTaskbars[i]) ShowWindow(g_secondaryTaskbars[i], SW_HIDE);
}

// --- window enumeration: "is any real user window visible?" ------------------

bool IsWindowCloaked(HWND hwnd) {
    BOOL cloaked = FALSE;
    if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))))
        return cloaked != 0;
    return false;
}

// Class names that never count as "a user window is open".
// Kept in sync with the original AHK sysClasses list.
const wchar_t* const kSystemClasses[] = {
    L"Progman",
    L"WorkerW",
    L"Shell_TrayWnd",
    L"Shell_SecondaryTrayWnd",
    L"Windows.UI.Core.CoreWindow",
    L"ApplicationFrameWindow",            // handled specially below
    L"TopLevelWindowForOverflowXamlIsland",
    L"Shell_InputSwitchTopLevelWindow",
    L"XamlExplorerHostIslandWindow",
    L"InputApp",
    L"LockApp",
    L"Windows.UI.Composition.DesktopWindowContentBridge",
    L"EdgeUiInputTopWndClass",
    L"NarratorHelperWindow",
    L"Shell_LightDismissOverlay",
    L"SearchHost",
    L"NotifyIconOverflowWindow",
};

struct EnumCtx {
    bool found = false;
};

BOOL CALLBACK EnumProc(HWND hwnd, LPARAM lp) {
    auto* ctx = reinterpret_cast<EnumCtx*>(lp);

    if (!IsWindowVisible(hwnd)) return TRUE;

    // Minimized windows don't count as "visible on desktop".
    if (IsIconic(hwnd)) return TRUE;

    wchar_t cls[128] = {};
    if (GetClassNameW(hwnd, cls, _countof(cls)) <= 0) return TRUE;

    for (const wchar_t* sc : kSystemClasses) {
        if (wcscmp(cls, sc) == 0) return TRUE;
    }

    wchar_t title[256] = {};
    GetWindowTextW(hwnd, title, _countof(title));
    if (title[0] == L'\0') return TRUE;
    if (wcscmp(title, L"Program Manager") == 0) return TRUE;
    if (wcscmp(title, L"Windows Shell Experience Host") == 0) return TRUE;

    LONG exStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW) return TRUE;

    if (IsWindowCloaked(hwnd)) return TRUE;

    ctx->found = true;
    return FALSE; // stop enumeration
}

// Class names that indicate an interactive flyout is open (Start, Search,
// notification center, quick settings, etc.). If any is open, we force the
// taskbar to stay visible so the user can click into it.
const wchar_t* const kFlyoutClasses[] = {
    L"NotifyIconOverflowWindow",
    L"TopLevelWindowForOverflowXamlIsland",
    L"Shell_InputSwitchTopLevelWindow",
    L"TaskListThumbnailWnd",
    L"ControlCenterWindow",
};

bool IsAnyFlyoutOpen() {
    for (const wchar_t* cls : kFlyoutClasses) {
        if (FindWindowW(cls, nullptr) != nullptr) return true;
    }

    // Windows.UI.Core.CoreWindow is used by Start/Search/etc.
    HWND core = nullptr;
    while ((core = FindWindowExW(nullptr, core, L"Windows.UI.Core.CoreWindow", nullptr)) != nullptr) {
        if (IsWindowCloaked(core)) continue;
        wchar_t title[256] = {};
        GetWindowTextW(core, title, _countof(title));
        if (wcscmp(title, L"Cortana") == 0) continue;
        if (wcscmp(title, L"Search")  == 0) continue;
        return true;
    }

    return false;
}

bool HasVisibleWindows() {
    if (IsAnyFlyoutOpen()) return true;
    EnumCtx ctx;
    EnumWindows(EnumProc, reinterpret_cast<LPARAM>(&ctx));
    return ctx.found;
}

// --- hover detection ---------------------------------------------------------

bool IsPointOnTaskbar(POINT p) {
    auto inRect = [&](HWND h) -> bool {
        if (!h) return false;
        RECT r{};
        if (!GetWindowRect(h, &r)) return false;
        return p.x >= r.left && p.x < r.right && p.y >= r.top && p.y < r.bottom;
    };
    if (inRect(g_mainTaskbar)) return true;
    for (size_t i = 0; i < g_secondaryCount; ++i)
        if (inRect(g_secondaryTaskbars[i])) return true;
    return false;
}

bool IsPointOnFlyout(POINT p) {
    HWND hwnd = WindowFromPoint(p);
    if (!hwnd) return false;

    HWND root = GetAncestor(hwnd, GA_ROOT);
    if (!root) return false;

    static const wchar_t* kFlyoutish[] = {
        L"NotifyIconOverflowWindow",
        L"TopLevelWindowForOverflowXamlIsland",
        L"Windows.UI.Core.CoreWindow",
        L"XamlExplorerHostIslandWindow",
        L"Shell_InputSwitchTopLevelWindow",
        L"TaskListThumbnailWnd",
        L"ControlCenterWindow",
    };
    wchar_t cls[128] = {};
    if (GetClassNameW(root, cls, _countof(cls)) > 0) {
        for (const wchar_t* c : kFlyoutish) {
            if (wcscmp(cls, c) == 0) return true;
        }
    }

    // Also treat any window owned by a taskbar as a flyout.
    HWND owner = GetWindow(root, GW_OWNER);
    if (owner) {
        if (ClassNameMatchesAny(owner, {L"Shell_TrayWnd", L"Shell_SecondaryTrayWnd"}))
            return true;
    }
    return false;
}

bool IsMouseInTaskbarZone() {
    POINT p{};
    if (!GetCursorPos(&p)) return false;

    // Bottom-edge strip on any monitor.
    struct EnumMonCtx { POINT pt; bool hit; };
    EnumMonCtx mctx{p, false};
    EnumDisplayMonitors(nullptr, nullptr,
        [](HMONITOR mon, HDC, LPRECT, LPARAM lp) -> BOOL {
            auto* c = reinterpret_cast<EnumMonCtx*>(lp);
            MONITORINFO mi{sizeof(mi)};
            if (!GetMonitorInfoW(mon, &mi)) return TRUE;
            const RECT& r = mi.rcMonitor;
            if (c->pt.x >= r.left && c->pt.x < r.right &&
                c->pt.y >= r.bottom - kHoverZonePx) {
                c->hit = true;
                return FALSE;
            }
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&mctx));
    if (mctx.hit) return true;

    if (IsPointOnTaskbar(p)) return true;
    if (IsPointOnFlyout(p))  return true;
    return false;
}

// --- state machine -----------------------------------------------------------

void UpdateTaskbar() {
    const bool hasWindows = HasVisibleWindows();
    g_lastWindowState = hasWindows;

    if (hasWindows) {
        g_taskbarHidden = false;
        g_hoverRevealed = false;
        ShowTaskbar();
    } else {
        g_taskbarHidden = true;
        g_hoverRevealed = false;
        HideTaskbar();
    }
}

void MainLoop() {
    const bool currentState = HasVisibleWindows();
    if (currentState != g_lastWindowState) {
        g_lastWindowState = currentState;
        UpdateTaskbar();
        return;
    }

    if (!g_taskbarHidden) return;

    const bool mouseInZone = IsMouseInTaskbarZone();
    if (mouseInZone && !g_hoverRevealed) {
        ShowTaskbar();
        g_hoverRevealed = true;
    } else if (!mouseInZone && g_hoverRevealed) {
        HideTaskbar();
        g_hoverRevealed = false;
    }
}

void OnShellEvent(WPARAM wParam) {
    switch (wParam) {
        case HSHELL_WINDOWCREATED:
        case HSHELL_WINDOWDESTROYED:
        case HSHELL_WINDOWACTIVATED:
        case HSHELL_REDRAW:
        case HSHELL_RUDEAPPACTIVATED: // 0x8004
            // Debounce: if several events fire back-to-back, only update once.
            SetTimer(g_hWnd, kTimerIdDebounce, kDebounceMs, nullptr);
            break;
        default:
            break;
    }
}

// --- tray --------------------------------------------------------------------

void AddTrayIcon() {
    g_nid.cbSize           = sizeof(g_nid);
    g_nid.hWnd             = g_hWnd;
    g_nid.uID              = 1;
    g_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = kTrayMsg;
    g_nid.hIcon            = LoadIconW(nullptr, IDI_APPLICATION);
    wcsncpy_s(g_nid.szTip, L"TaskbarHider — right-click for menu", _TRUNCATE);
    if (Shell_NotifyIconW(NIM_ADD, &g_nid)) {
        g_trayAdded = true;
        g_nid.uVersion = NOTIFYICON_VERSION_4;
        Shell_NotifyIconW(NIM_SETVERSION, &g_nid);
    }
}

void RemoveTrayIcon() {
    if (g_trayAdded) {
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        g_trayAdded = false;
    }
}

void ShowTrayMenu() {
    POINT pt{};
    GetCursorPos(&pt);

    HMENU menu = CreatePopupMenu();
    if (!menu) return;

    AppendMenuW(menu, MF_STRING, IDM_TRAY_SHOW_TASKBAR, L"Restore taskbar now");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, IDM_TRAY_ABOUT, L"About TaskbarHider");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, IDM_TRAY_EXIT,  L"Exit (and restore taskbar)");

    // Required so the menu dismisses when clicking elsewhere.
    SetForegroundWindow(g_hWnd);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN,
                   pt.x, pt.y, 0, g_hWnd, nullptr);
    PostMessageW(g_hWnd, WM_NULL, 0, 0);
    DestroyMenu(menu);
}

// --- window proc -------------------------------------------------------------

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == g_shellHookMsg && g_shellHookMsg != 0) {
        OnShellEvent(wParam);
        return 0;
    }

    switch (msg) {
    case WM_CREATE: {
        g_shellHookMsg = RegisterWindowMessageW(L"SHELLHOOK");
        g_shellHookRegistered = RegisterShellHookWindow(hWnd) != FALSE;

        RefreshTaskbarHandles();
        AddTrayIcon();

        RegisterHotKey(hWnd, kEmergencyHotkeyId,
                       MOD_CONTROL | MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, 'T');

        SetTimer(hWnd, kTimerIdMain,    kMainTimerMs,    nullptr);
        SetTimer(hWnd, kTimerIdRefresh, kRefreshTimerMs, nullptr);

        UpdateTaskbar();
        return 0;
    }

    case WM_TIMER:
        switch (wParam) {
        case kTimerIdMain:
            MainLoop();
            return 0;
        case kTimerIdRefresh:
            RefreshTaskbarHandles();
            return 0;
        case kTimerIdDebounce:
            KillTimer(hWnd, kTimerIdDebounce);
            UpdateTaskbar();
            return 0;
        }
        break;

    case WM_HOTKEY:
        if (wParam == kEmergencyHotkeyId) {
            PostMessageW(hWnd, WM_CLOSE, 0, 0);
            return 0;
        }
        break;

    case WM_DISPLAYCHANGE:
    case WM_DPICHANGED:
        RefreshTaskbarHandles();
        UpdateTaskbar();
        return 0;

    case WM_QUERYENDSESSION:
        ShowTaskbar();
        return TRUE;

    case WM_ENDSESSION:
        ShowTaskbar();
        return 0;

    case kTrayMsg: {
        const UINT event = LOWORD(lParam);
        switch (event) {
        case WM_LBUTTONUP:
        case WM_CONTEXTMENU:
        case NIN_SELECT:
        case NIN_KEYSELECT:
        case WM_RBUTTONUP:
            ShowTrayMenu();
            return 0;
        }
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_TRAY_EXIT:
            PostMessageW(hWnd, WM_CLOSE, 0, 0);
            return 0;
        case IDM_TRAY_SHOW_TASKBAR:
            RefreshTaskbarHandles();
            ShowTaskbar();
            g_taskbarHidden = false;
            g_hoverRevealed = false;
            return 0;
        case IDM_TRAY_ABOUT:
            MessageBoxW(hWnd,
                L"TaskbarHider\n\n"
                L"Auto-hides the Windows taskbar when no windows are open.\n"
                L"Hover the bottom edge to reveal it.\n\n"
                L"Emergency exit: Ctrl+Alt+Shift+T\n\n"
                L"https://github.com/babaJaan01/taskbarhider",
                L"About TaskbarHider",
                MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;

    case WM_DESTROY:
        KillTimer(hWnd, kTimerIdMain);
        KillTimer(hWnd, kTimerIdRefresh);
        KillTimer(hWnd, kTimerIdDebounce);
        UnregisterHotKey(hWnd, kEmergencyHotkeyId);
        if (g_shellHookRegistered) {
            DeregisterShellHookWindow(hWnd);
            g_shellHookRegistered = false;
        }
        RemoveTrayIcon();
        ShowTaskbar();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

} // namespace

// --- entry point -------------------------------------------------------------

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    g_hInstance = hInstance;

    // Single-instance guard.
    HANDLE mutex = CreateMutexW(nullptr, FALSE, kMutexName);
    if (!mutex || GetLastError() == ERROR_ALREADY_EXISTS) {
        // Bring the existing instance's tray menu up isn't trivial; just exit quietly.
        return 0;
    }

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    WNDCLASSEXW wc{sizeof(wc)};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = kWindowClass;
    wc.hIcon         = LoadIconW(nullptr, IDI_APPLICATION);
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    if (!RegisterClassExW(&wc)) {
        ReleaseMutex(mutex);
        CloseHandle(mutex);
        return 1;
    }

    // Message-only-ish hidden window. We need a real top-level window (not
    // HWND_MESSAGE) so that RegisterShellHookWindow and RegisterHotKey work.
    g_hWnd = CreateWindowExW(
        0, kWindowClass, kWindowTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 0, 0,
        nullptr, nullptr, hInstance, nullptr);

    if (!g_hWnd) {
        ReleaseMutex(mutex);
        CloseHandle(mutex);
        return 1;
    }

    // Never show the window — it's just for receiving messages.
    ShowWindow(g_hWnd, SW_HIDE);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    ReleaseMutex(mutex);
    CloseHandle(mutex);
    return static_cast<int>(msg.wParam);
}
