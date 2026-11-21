# Read-only inspection of the registered Fast Track ASIO COM server.
# No COM activation, DLL loading, driver changes, or audio initialization.
$ErrorActionPreference = 'Stop'
$clsid = '{479DE6B0-18B0-4e18-A91F-AD75AAA570FE}'
Write-Host 'SR850 Fast Track COM server inspection (read-only)'
Write-Host ('CLSID: ' + $clsid)
foreach ($view in @('Registry64','Registry32')) {
    $regView = [Microsoft.Win32.RegistryView]::$view
    $root = [Microsoft.Win32.RegistryKey]::OpenBaseKey([Microsoft.Win32.RegistryHive]::ClassesRoot, $regView)
    try {
        $key = $root.OpenSubKey(('CLSID\' + $clsid + '\InprocServer32'))
        if ($null -eq $key) {
            Write-Host ($view + ': no InprocServer32 registration')
            continue
        }
        try {
            $dll = [string]$key.GetValue('')
            $threading = [string]$key.GetValue('ThreadingModel')
            Write-Host ($view + ': COM server registered')
            Write-Host ('  DLL file: ' + [System.IO.Path]::GetFileName($dll))
            Write-Host ('  Threading model: ' + $threading)
            if ([string]::IsNullOrWhiteSpace($dll)) {
                Write-Host '  DLL path missing'
            } else {
                $expanded = [Environment]::ExpandEnvironmentVariables($dll.Trim('"'))
                Write-Host ('  File exists: ' + [System.IO.File]::Exists($expanded))
                if ([System.IO.File]::Exists($expanded)) {
                    $version = [System.Diagnostics.FileVersionInfo]::GetVersionInfo($expanded)
                    Write-Host ('  File version: ' + $version.FileVersion)
                }
            }
        } finally { $key.Dispose() }
    } finally { $root.Dispose() }
}
Write-Host 'Inspection complete. Full paths intentionally omitted.'
