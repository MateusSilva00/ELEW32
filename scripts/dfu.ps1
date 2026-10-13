param(
    [ValidateSet('List', 'Flash')]
    [string]$Action = 'List',
    [ValidateSet('a', 'b', 'c', 'd')]
    [string]$Question
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$dfuUtil = Join-Path $projectRoot 'out/tools/dfu-util-0.11-binaries/win64/dfu-util.exe'

if (-not (Test-Path -LiteralPath $dfuUtil)) {
    throw 'Execute a tarefa DFU: Setup antes de usar o gravador USB.'
}

# A virgula restringe a selecao ao VID/PID do modo DFU, sem outros dispositivos.
if ($Action -eq 'List') {
    & $dfuUtil -d ',0483:df11' -a 0 -l
    exit $LASTEXITCODE
}

if (-not $Question) { throw 'Informe -Question a, b, c ou d para selecionar o firmware.' }
$Question = $Question.ToLowerInvariant()
if (-not (Test-Path -LiteralPath (Join-Path $projectRoot "atividade3/questao_$Question/CMakeLists.txt"))) {
    throw "A questao ($Question) ainda nao possui firmware implementado."
}
$firmware = Join-Path $projectRoot "out/build/questao_$Question/questao_$Question.bin"
$readback = Join-Path $projectRoot "out/build/questao_$Question/dfu-readback.bin"
if (-not (Test-Path -LiteralPath $firmware)) {
    throw 'Firmware ausente. Compile a questao selecionada com scripts/project.ps1 antes de gravar.'
}
Write-Host "Gravando questao ($Question): $firmware"
$imageBytes = [IO.File]::ReadAllBytes($firmware)
if ($imageBytes.Length -lt 8 -or $imageBytes.Length -gt 524288) {
    throw 'Tamanho de firmware invalido para a STM32F411CEU6.'
}
$initialStack = [BitConverter]::ToUInt32($imageBytes, 0)
$resetVector = [BitConverter]::ToUInt32($imageBytes, 4)
if ($initialStack -ne 0x20020000 -or ($resetVector -band 1) -ne 1 -or
    $resetVector -lt 0x08000000 -or $resetVector -ge 0x08080000) {
    throw 'Tabela de vetores incompativel com o firmware deste projeto.'
}

# Alt 0 e a Flash interna. Nao altera option bytes nem solicita mass erase.
& $dfuUtil -d ',0483:df11' -a 0 -s '0x08000000' -D $firmware
if ($LASTEXITCODE -ne 0) { throw 'Falha na gravacao DFU. Confira modo DFU e driver USB.' }

# Leitura de volta verifica os bytes gravados antes do reset manual.
# dfu-util recusa sobrescrever o arquivo de uma verificacao anterior.
if (Test-Path -LiteralPath $readback) {
    Remove-Item -LiteralPath $readback -ErrorAction Stop
}
& $dfuUtil -d ',0483:df11' -a 0 -s "0x08000000:$($imageBytes.Length)" -U $readback
if ($LASTEXITCODE -ne 0) { throw 'Falha na leitura de verificacao da Flash.' }
if ((Get-FileHash -LiteralPath $firmware -Algorithm SHA256).Hash -ne
    (Get-FileHash -LiteralPath $readback -Algorithm SHA256).Hash) {
    throw 'A leitura da Flash difere do firmware compilado.'
}
Write-Host 'Gravacao e verificacao concluidas. Solte BOOT0 e pressione RESET para executar.'
