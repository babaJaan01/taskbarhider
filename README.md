# TaskbarHider

TaskbarHider hides the Windows taskbar when no normal app windows are visible,
and reveals it when you hover the bottom edge, open an app, or optionally when
an app needs attention.

It uses a C++ Win32 shell host with a Rust core for state-machine and rules
logic.

## Download

Download from [Releases](https://github.com/babaJaan01/taskbarhider/releases):

- `TaskbarHider-Setup-x64.exe` (recommended for almost all PCs)
- `TaskbarHider-Setup-x86.exe` (only for 32-bit Windows)

## Install

1. Run the setup file.
2. Pick install folder (default: `%LocalAppData%\\Programs\\TaskbarHider`).
3. Choose startup and app-attention options.
4. Finish setup.

Runtime config is stored beside the installed app as `taskbarhider.toml`:

```toml
# Changes apply the next time TaskbarHider starts.
# If startup is enabled, that means the next Windows sign-in or reboot.
start_on_startup = true

# Show the taskbar when an app requests attention.
show_on_app_attention = true
```

TaskbarHider reads this file only when it starts, keeping idle overhead low.

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
