# SR850 ASIO diagnostics (Windows)
These utilities are **read-only diagnostics**, not an ASIO bridge or installer.
They do not modify the Windows registry, install drivers, or start audio streams.

## Identify registered ASIO drivers
Extract the GitHub Actions artifact `SR850-ASIO-Diagnostics-Windows`.
Open PowerShell in the extracted folder and run:
```powershell
.\asio_registry.exe
```
The output lists registered 32-bit and 64-bit ASIO names and CLSIDs.
For additional registration information, run:
```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\diagnose-asio.ps1
```

## Optional: probe activation
**Only if you are comfortable testing a driver** and have closed all audio applications:
```powershell
.\asio_activation_probe.exe "{PASTE-A-REAL-CLSID-FROM-OUTPUT}"
```
This asks Windows to instantiate the COM component and immediately releases it.
It does **not** call ASIO init/start, query channels, create buffers, or process audio.
Some legacy drivers may crash or hang during activation. Do not use this step during
a recording session; reboot if a driver becomes unresponsive.

## Limitations
- The GitHub runner has no access to your Fast Track MKII.
- Registry discovery does not establish hardware compatibility.
- ASIO applications must use a genuine ASIO driver; this package provides none.
- The current SR850 Windows audio correction remains separate.
- Do not share logs containing private file paths or personal information.
