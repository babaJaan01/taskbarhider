# TaskbarHider uninstaller
# - Stops any running instance (and restores the taskbar)
# - Removes the Startup shortcut
# - Deletes the install directory

#Requires -Version 5.1

$ErrorActionPreference = 'Continue'

function Write-Step($msg) { Write-Host "==> $msg" -ForegroundColor Cyan }
function Write-Ok($msg)   { Write-Host "    $msg" -ForegroundColor Green }

$installDir   = Join-Path $env:LOCALAPPDATA 'TaskbarHider'
$startup      = [Environment]::GetFolderPath('Startup')
$shortcutPath = Join-Path $startup 'TaskbarHider.lnk'

Write-Step 'Stopping TaskbarHider (this will restore the taskbar)'
Get-CimInstance Win32_Process -Filter "Name='AutoHotkey64.exe' OR Name='AutoHotkey32.exe'" -ErrorAction SilentlyContinue |
    Where-Object { $_.CommandLine -like "*TaskbarHider.ahk*" } |
    ForEach-Object {
        try { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue } catch {}
    }

# Safety net: force-show the main + secondary taskbars in case something was stuck
Add-Type -Namespace TH -Name Win -MemberDefinition @"
    [System.Runtime.InteropServices.DllImport("user32.dll", CharSet=System.Runtime.InteropServices.CharSet.Auto)]
    public static extern System.IntPtr FindWindow(string cls, string name);
    [System.Runtime.InteropServices.DllImport("user32.dll", CharSet=System.Runtime.InteropServices.CharSet.Auto)]
    public static extern System.IntPtr FindWindowEx(System.IntPtr p, System.IntPtr c, string cls, string name);
    [System.Runtime.InteropServices.DllImport("user32.dll")]
    public static extern bool ShowWindow(System.IntPtr hWnd, int nCmdShow);
"@
$main = [TH.Win]::FindWindow('Shell_TrayWnd', $null)
if ($main -ne [IntPtr]::Zero) { [void][TH.Win]::ShowWindow($main, 5) }
$cur = [IntPtr]::Zero
while ($true) {
    $cur = [TH.Win]::FindWindowEx([IntPtr]::Zero, $cur, 'Shell_SecondaryTrayWnd', $null)
    if ($cur -eq [IntPtr]::Zero) { break }
    [void][TH.Win]::ShowWindow($cur, 5)
}
Write-Ok 'Taskbar restored.'

if (Test-Path $shortcutPath) {
    Remove-Item $shortcutPath -Force
    Write-Ok "Removed startup shortcut: $shortcutPath"
}

if (Test-Path $installDir) {
    Remove-Item $installDir -Recurse -Force
    Write-Ok "Removed install dir: $installDir"
}

Write-Host ''
Write-Host 'TaskbarHider has been uninstalled.' -ForegroundColor Green
