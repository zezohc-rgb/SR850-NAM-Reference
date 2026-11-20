param([string]$LogPath = "$env:TEMP\SR850-APO-install.log")
$ErrorActionPreference = 'Stop'
Start-Transcript -Path $LogPath -Force | Out-Null
try {
  $roots = @("$env:ProgramFiles\EqualizerAPO", "${env:ProgramFiles(x86)}\EqualizerAPO")
  foreach ($root in $roots) {
    if (Test-Path (Join-Path $root 'Configurator.exe')) { exit 0 }
  }
  # Equalizer APO is NOT in the winget community catalog (interactive device selection).
  # Download the genuine SourceForge installer on the user's PC and verify the publisher SHA256.
  $expected = '7403be7427bbe1936a40dded082829b6e217fc4f5990fee5cba501f0ae055afa'
  $download = Join-Path $env:TEMP 'EqualizerAPO-x64-1.4.2.exe'
  $urls = @(
    'https://downloads.sourceforge.net/project/equalizerapo/1.4.2/EqualizerAPO-x64-1.4.2.exe',
    'https://sourceforge.net/projects/equalizerapo/files/1.4.2/EqualizerAPO-x64-1.4.2.exe/download'
  )
  [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
  $valid = $false
  foreach ($url in $urls) {
    try {
      Write-Host "Baixando Equalizer APO oficial: $url"
      & curl.exe --fail --location --retry 2 --max-time 120 --user-agent 'Mozilla/5.0' --output $download $url
      if ($LASTEXITCODE -ne 0) { continue }
      $hash = (Get-FileHash -LiteralPath $download -Algorithm SHA256).Hash.ToLowerInvariant()
      if ($hash -eq $expected) { $valid = $true; break }
      Write-Warning "SHA256 divergente: $hash"
    } catch { Write-Warning $_ }
  }
  if (-not $valid) { throw 'Falha ao baixar e autenticar o Equalizer APO oficial. Verifique a conexao e o log.' }
  Write-Host 'SHA256 oficial confirmado. Iniciando instalador interativo.'
  # Do NOT use /S: Equalizer APO must let the user select the M-Audio playback device.
  $process = Start-Process -FilePath $download -Wait -PassThru
  if ($process.ExitCode -ne 0) { throw "Instalador do Equalizer APO retornou $($process.ExitCode)" }
  foreach ($root in $roots) {
    if (Test-Path (Join-Path $root 'Configurator.exe')) { exit 0 }
  }
  throw 'Instalador terminou, mas o Configurator.exe nao foi encontrado.'
} catch {
  Write-Host "ERRO: $_"
  exit 1
} finally {
  Stop-Transcript | Out-Null
}
