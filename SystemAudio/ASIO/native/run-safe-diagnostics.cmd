@echo off
setlocal
cd /d "%~dp0"
echo SR850 ASIO Diagnostics - read-only
echo.
if not exist "asio_registry.exe" (
  echo ERROR: asio_registry.exe not found in this folder.
  pause
  exit /b 1
)
asio_registry.exe
echo.
echo --- Extended registry inspection ---
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0diagnose-asio.ps1"
echo.
echo.
echo --- Fast Track COM server information ---
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0inspect-fast-track-com.ps1"
echo.
echo Finished. No driver was activated and no registry entries were changed.
echo You may copy the driver names and CLSIDs shown above.
pause
