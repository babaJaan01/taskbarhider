# TaskbarHider installer
# - Ensures AutoHotkey v2 is installed (via winget, or prompts the user)
# - Copies TaskbarHider.ahk to %LOCALAPPDATA%\TaskbarHider
# - Creates a Startup shortcut that runs the script with the signed AutoHotkey64.exe
#
# This intentionally does NOT ship a compiled .exe, which is why Microsoft Defender
# won't flag it. The AHK interpreter (AutoHotkey64.exe) is signed by AutoHotkey
# Foundation Limited and already trusted by Defender.

#Requires -Version 5.1

$ErrorActionPreference = 'Stop'

function Write-Step($msg) { Write-Host "==> $msg" -ForegroundColor Cyan }
function Write-Ok($msg)   { Write-Host "    $msg" -ForegroundColor Green }
function Write-Warn2($msg){ Write-Host "    $msg" -ForegroundColor Yellow }
function Write-Err($msg)  { Write-Host "!!! $msg" -ForegroundColor Red }

# --- locate script source ----------------------------------------------------
$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$sourceAhk  = Join-Path $scriptRoot 'TaskbarHider.ahk'
if (-not (Test-Path $sourceAhk)) {
    Write-Err "TaskbarHider.ahk not found next to install.ps1 (looked in $scriptRoot)."
    exit 1
}

# --- locate or install AutoHotkey v2 -----------------------------------------
function Find-AutoHotkeyV2 {
    $candidates = @(
        "$env:ProgramFiles\AutoHotkey\v2\AutoHotkey64.exe",
        "$env:ProgramFiles\AutoHotkey\v2\AutoHotkey32.exe",
        "${env:ProgramFiles(x86)}\AutoHotkey\v2\AutoHotkey64.exe",
        "${env:ProgramFiles(x86)}\AutoHotkey\v2\AutoHotkey32.exe",
        "$env:LOCALAPPDATA\Programs\AutoHotkey\v2\AutoHotkey64.exe"
    )
    foreach ($c in $candidates) {
        if ($c -and (Test-Path $c)) { return $c }
    }
    return $null
}

Write-Step 'Checking for AutoHotkey v2'
$ahkExe = Find-AutoHotkeyV2
if ($ahkExe) {
    Write-Ok "Found $ahkExe"
} else {
    Write-Warn2 'AutoHotkey v2 not installed.'
    $winget = Get-Command winget -ErrorAction SilentlyContinue
    if ($winget) {
        Write-Step 'Installing AutoHotkey v2 via winget (this may take ~30s)'
        try {
            winget install --id AutoHotkey.AutoHotkey -e --silent --accept-source-agreements --accept-package-agreements | Out-Host
        } catch {
            Write-Err "winget install failed: $($_.Exception.Message)"
        }
        $ahkExe = Find-AutoHotkeyV2
    }
    if (-not $ahkExe) {
        Write-Err 'Could not install AutoHotkey v2 automatically.'
        Write-Host ''
        Write-Host 'Please install AutoHotkey v2 manually from:' -ForegroundColor Yellow
        Write-Host '  https://www.autohotkey.com/download/' -ForegroundColor Yellow
        Write-Host 'Then run install.ps1 again.'
        exit 1
    }
    Write-Ok "Installed: $ahkExe"
}

# --- copy script to a stable location ----------------------------------------
$installDir = Join-Path $env:LOCALAPPDATA 'TaskbarHider'
if (-not (Test-Path $installDir)) {
    New-Item -ItemType Directory -Path $installDir | Out-Null
}
$destAhk = Join-Path $installDir 'TaskbarHider.ahk'
Copy-Item -Path $sourceAhk -Destination $destAhk -Force
Write-Ok "Copied script to $destAhk"

# --- create Startup shortcut -------------------------------------------------
$startup     = [Environment]::GetFolderPath('Startup')
$shortcutPath = Join-Path $startup 'TaskbarHider.lnk'

$wshell   = New-Object -ComObject WScript.Shell
$shortcut = $wshell.CreateShortcut($shortcutPath)
$shortcut.TargetPath       = $ahkExe
$shortcut.Arguments        = "`"$destAhk`""
$shortcut.WorkingDirectory = $installDir
$shortcut.WindowStyle      = 7   # minimized
$shortcut.IconLocation     = "$ahkExe,0"
$shortcut.Description      = 'TaskbarHider - auto-hide the Windows taskbar when no windows are open'
$shortcut.Save()
Write-Ok "Created startup shortcut at $shortcutPath"

# --- launch now --------------------------------------------------------------
Write-Step 'Starting TaskbarHider'

# Kill any already-running instance so the fresh copy takes over
Get-CimInstance Win32_Process -Filter "Name='AutoHotkey64.exe' OR Name='AutoHotkey32.exe'" -ErrorAction SilentlyContinue |
    Where-Object { $_.CommandLine -like "*TaskbarHider.ahk*" } |
    ForEach-Object {
        try { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue } catch {}
    }

Start-Process -FilePath $ahkExe -ArgumentList "`"$destAhk`"" -WorkingDirectory $installDir
Write-Ok 'TaskbarHider is running.'
Write-Host ''
Write-Host 'Install complete.' -ForegroundColor Green
Write-Host '  Emergency exit / restore taskbar: Ctrl+Alt+Shift+T'
Write-Host "  Uninstall:                         powershell -File `"$scriptRoot\uninstall.ps1`""
