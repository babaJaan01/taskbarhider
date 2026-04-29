# TaskbarHider

TaskbarHider hides the Windows taskbar when no normal app windows are visible,
and reveals it when you hover the bottom edge or open an app.

## Download

Download from [Releases](https://github.com/babaJaan01/taskbarhider/releases):

- `TaskbarHider-Setup-x64.exe` (recommended for almost all PCs)
- `TaskbarHider-Setup-x86.exe` (only for 32-bit Windows)

## Install

1. Run the setup file.
2. Pick install folder (default: `%LocalAppData%\\Programs\\TaskbarHider`).
3. Choose whether to start TaskbarHider when you sign in.
4. Finish setup.

## Uninstall

Open **Settings -> Apps -> Installed apps**, select **TaskbarHider**, then click **Uninstall**.

## Verify your download (SHA256SUMS)

Each release includes `SHA256SUMS.txt`, which lists trusted SHA-256 hashes for
the installer files.

Why verify:
- confirms the download is complete and unmodified
- helps detect tampering

PowerShell example:

```powershell
Get-FileHash .\TaskbarHider-Setup-x64.exe -Algorithm SHA256
```

Compare that hash to the `TaskbarHider-Setup-x64.exe` line in `SHA256SUMS.txt`.
They must match exactly.

Note: setup does not currently auto-verify against `SHA256SUMS.txt`, so this is
a manual check.

## Emergency exit

If needed, press `Ctrl+Alt+Shift+T` or end `TaskbarHider.exe` in Task Manager.

## License

MIT
