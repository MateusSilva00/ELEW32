param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('a', 'b', 'c', 'd')]
    [string]$Question,
    [ValidateSet('Configure', 'Build', 'Clean', 'Flash')]
    [string]$Action = 'Build'
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$Question = $Question.ToLowerInvariant()
$sourceDirectory = Join-Path $projectRoot "atividade3/questao_$Question"
$buildDirectory = Join-Path $projectRoot "out/build/questao_$Question"
if (-not (Test-Path -LiteralPath (Join-Path $sourceDirectory 'CMakeLists.txt'))) {
    throw "A questao ($Question) ainda nao possui firmware implementado."
}
Write-Host "Questao ($Question): $Action - $buildDirectory"

if ($Action -eq 'Clean') {
    if (-not (Test-Path -LiteralPath (Join-Path $buildDirectory 'CMakeCache.txt'))) {
        throw "A questao ($Question) ainda nao foi configurada."
    }
    & cmake --build $buildDirectory --target clean
    if ($LASTEXITCODE -ne 0) { throw 'Falha ao limpar o build.' }
    exit 0
}

& cmake -S $sourceDirectory -B $buildDirectory -G Ninja `
    -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
if ($LASTEXITCODE -ne 0) { throw 'Falha ao configurar o projeto.' }
if ($Action -eq 'Configure') { exit 0 }

& cmake --build $buildDirectory
if ($LASTEXITCODE -ne 0) { throw 'Falha ao compilar o firmware.' }
if ($Action -eq 'Flash') {
    & (Join-Path $PSScriptRoot 'dfu.ps1') -Action Flash -Question $Question
}
