param([string]$LogPath = "$env:TEMP\SR850-APO-install.log")
$ErrorActionPreference = 'Stop'
Start-Transcript -Path $LogPath -Force | Out-Null
try {
  $roots = @("$env:ProgramFiles\EqualizerAPO", "${env:ProgramFiles(x86)}\EqualizerAPO")
  foreach ($root in $roots) {
    if (Test-Path (Join-Path $root 'Configurator.exe')) { exit 0 }
  }
  $winget = Get-Command winget.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -First 1
  if (-not $winget) {
    $packages = @(Get-ChildItem "$env:ProgramFiles\WindowsApps\Microsoft.DesktopAppInstaller_*_x64__8wekyb3d8bbwe\winget.exe" -ErrorAction SilentlyContinue | Sort-Object FullName -Descending)
    if ($packages.Count) { $winget = $packages[0].FullName }
  }
  if (-not $winget) { throw "winget indisponivel. Instale ou atualize App Installer do Windows." }
  & $winget source update --name winget
  & $winget install --id EqualizerAPO.EqualizerAPO --exact --source winget --accept-package-agreements --accept-source-agreements --silent --disable-interactivity
  $code=$LASTEXITCODE
  if ($code -ne 0) { throw "winget falhou: codigo $code" }
  foreach ($root in $roots) {
    if (Test-Path (Join-Path $root 'Configurator.exe')) { exit 0 }
  }
  throw "Instalacao nao detectada. Verifique o log e o dispositivo de audio."
} catch {
  Write-Error $_
  exit 1
} finally {
  Stop-Transcript | Out-Null
}
