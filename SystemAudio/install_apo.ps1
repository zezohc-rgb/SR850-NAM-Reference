param([string]$LogPath = "$env:TEMP\SR850-APO-install.log", [string]$InstallerDirectory = "")
$ErrorActionPreference = 'Stop'
function Find-EqualizerAPO {
  $locations = @(
    "$env:ProgramW6432\EqualizerAPO",
    "$env:ProgramFiles\EqualizerAPO",
    "${env:ProgramFiles(x86)}\EqualizerAPO",
    "C:\Program Files\EqualizerAPO",
    "C:\Program Files (x86)\EqualizerAPO"
  )
  foreach ($key in @('HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*','HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*')) {
    foreach ($app in @(Get-ItemProperty $key -ErrorAction SilentlyContinue | Where-Object { $_.DisplayName -match '^Equalizer APO' })) {
      if ($app.InstallLocation) { $locations += $app.InstallLocation }
      Write-Host "Registro Equalizer APO: $($app.DisplayName), caminho=$($app.InstallLocation)"
    }
  }
  foreach ($location in ($locations | Select-Object -Unique)) {
    if (-not $location) { continue }
    Write-Host "Verificando instalacao em: $location"
    if (Test-Path (Join-Path $location 'config\config.txt')) {
      Write-Host "Configuracao do APO encontrada: $location"
      return $location
    }
  }
  return $null
}
Start-Transcript -Path $LogPath -Force | Out-Null
try {
  $installed = Find-EqualizerAPO
  if ($installed) { Write-Host "Equalizer APO ja instalado em $installed"; exit 0 }
  # Equalizer APO is NOT in the winget community catalog (interactive device selection).
  # Download the genuine SourceForge installer on the user's PC and verify the publisher SHA256.
  $expected = '7403be7427bbe1936a40dded082829b6e217fc4f5990fee5cba501f0ae055afa'
  $download = Join-Path $env:TEMP 'EqualizerAPO-x64-1.4.2.exe'
  $candidates = @(
    (Join-Path $InstallerDirectory 'EqualizerAPO-x64-1.4.2.exe'),
    (Join-Path (Join-Path $env:USERPROFILE 'Downloads') 'EqualizerAPO-x64-1.4.2.exe'),
    (Join-Path $env:TEMP 'EqualizerAPO-x64-1.4.2.exe')
  )
  $valid = $false
  foreach ($candidate in $candidates) {
    if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) { continue }
    $hash = (Get-FileHash -LiteralPath $candidate -Algorithm SHA256).Hash.ToLowerInvariant()
    Write-Host "Arquivo local encontrado: $candidate; SHA256=$hash"
    if ($hash -eq $expected) {
      $download = $candidate
      $valid = $true
      break
    }
  }
  if (-not $valid) {
    throw 'Equalizer APO oficial nao encontrado. Salve EqualizerAPO-x64-1.4.2.exe na pasta Downloads ou junto do Setup e execute novamente. Nenhum download automatico sera tentado.'
  }
  Write-Host 'SHA256 oficial confirmado. Iniciando instalador interativo.'
  # Do NOT use /S: Equalizer APO must let the user select the M-Audio playback device.
  $process = Start-Process -FilePath $download -Wait -PassThru
  if ($process.ExitCode -ne 0) { throw "Instalador do Equalizer APO retornou $($process.ExitCode)" }
  $installed = Find-EqualizerAPO
  if ($installed) { Write-Host "Equalizer APO instalado em $installed"; exit 0 }
  throw 'O instalador terminou sem detectar o Equalizer APO. Verifique se concluiu todas as telas (nao Cancelar). Consulte o registro de instalacao e os caminhos acima.'
} catch {
  Write-Host "ERRO: $_"
  exit 1
} finally {
  Stop-Transcript | Out-Null
}
