# SR850 ASIO diagnostic — read-only; does not install or modify drivers.
# Run in Windows PowerShell: powershell -ExecutionPolicy Bypass -File .\diagnose-asio.ps1
$ErrorActionPreference = 'Stop'
Write-Host 'SR850 Universal Audio Bridge - ASIO diagnostic'
Write-Host ('Windows: ' + [Environment]::OSVersion.VersionString)
Write-Host ('OS 64-bit: ' + [Environment]::Is64BitOperatingSystem)
Write-Host ('Process 64-bit: ' + [Environment]::Is64BitProcess)
$locations = @(
  @{ Name = '64-bit ASIO registry'; Path = 'Registry::HKEY_LOCAL_MACHINE\SOFTWARE\ASIO' },
  @{ Name = '32-bit ASIO registry'; Path = 'Registry::HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\ASIO' }
)
foreach ($location in $locations) {
  Write-Host ''
  Write-Host $location.Name
  if (-not (Test-Path -LiteralPath $location.Path)) {
    Write-Host '  No entries'
    continue
  }
  foreach ($entry in (Get-ChildItem -LiteralPath $location.Path -ErrorAction SilentlyContinue)) {
    $properties = Get-ItemProperty -LiteralPath $entry.PSPath
    Write-Host ('  Driver: ' + $entry.PSChildName)
    if ($properties.CLSID) {
      Write-Host ('  CLSID: ' + $properties.CLSID)
      $clsid = [string]$properties.CLSID
      foreach ($view in @('Registry::HKEY_LOCAL_MACHINE\SOFTWARE\Classes\CLSID\' + $clsid + '\InprocServer32',
                          'Registry::HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Classes\CLSID\' + $clsid + '\InprocServer32')) {
        if (Test-Path -LiteralPath $view) {
          $dll = (Get-Item -LiteralPath $view).GetValue('')
          if ($dll) { Write-Host ('  DLL: ' + $dll) }
        }
      }
    }
  }
}
Write-Host ''
Write-Host 'Read-only inspection finished. Do not post personal paths or serial numbers publicly.'
