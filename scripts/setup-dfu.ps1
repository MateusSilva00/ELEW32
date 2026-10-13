$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$toolsDirectory = Join-Path $projectRoot 'out/tools'
$archive = Join-Path $toolsDirectory 'dfu-util-0.11-binaries.tar.xz'
$expectedHash = '6450DE30A7DCD8D8C1273F43F0B153F054FD24D85F7F38296B1AD8EDBD2DDB25'
$downloadUrl = 'https://dfu-util.sourceforge.net/releases/dfu-util-0.11-binaries.tar.xz'

Get-Command cmake -ErrorAction Stop | Out-Null
New-Item -ItemType Directory -Force -Path $toolsDirectory | Out-Null
if (-not (Test-Path -LiteralPath $archive)) {
    Invoke-WebRequest -UseBasicParsing -Uri $downloadUrl -OutFile $archive
}
if ((Get-FileHash -Algorithm SHA256 -LiteralPath $archive).Hash -ne $expectedHash) {
    throw "Hash do pacote DFU incorreto. Remova apenas $archive e execute novamente."
}

# O CMake extrai XZ sem exigir um descompactador extra no PATH.
Push-Location $toolsDirectory
try {
    & cmake -E tar xf $archive
    if ($LASTEXITCODE -ne 0) { throw 'Falha ao extrair o dfu-util.' }
} finally {
    Pop-Location
}

& (Join-Path $toolsDirectory 'dfu-util-0.11-binaries/win64/dfu-util.exe') --version
if ($LASTEXITCODE -ne 0) { throw 'Falha ao executar o dfu-util.' }
Write-Host 'dfu-util preparado em out/tools. O pacote inclui as licencas originais.'
