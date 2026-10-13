param([string]$Compiler, [switch]$Setup)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
if (-not $Compiler) {
    $Compiler = Join-Path $projectRoot 'out/tools/tcc/tcc.exe'
    if ($Setup -and -not (Test-Path -LiteralPath $Compiler)) {
        $toolDirectory = Join-Path $projectRoot 'out/tools'
        New-Item -ItemType Directory -Force -Path $toolDirectory | Out-Null
        $archive = Join-Path $toolDirectory 'tcc-0.9.27-win64-bin.zip'
        Invoke-WebRequest -UseBasicParsing `
            -Uri 'https://download.savannah.gnu.org/releases/tinycc/tcc-0.9.27-win64-bin.zip' `
            -OutFile $archive
        $expected = '34A721949A2583FDFF725312DA092FA0F5F1F284B702E6F811C6954714FAABB2'
        if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $expected) {
            throw 'SHA-256 inesperado no pacote do compilador de testes.'
        }
        Expand-Archive -LiteralPath $archive -DestinationPath $toolDirectory -Force
    }
}
if (-not (Test-Path -LiteralPath $Compiler)) {
    throw 'Execute com -Setup para preparar o TCC, ou informe -Compiler com o caminho do GCC/Clang/TCC nativo.'
}
$testDirectory = Join-Path $projectRoot 'out/tests/questao_b'
New-Item -ItemType Directory -Force -Path $testDirectory | Out-Null
$testExecutable = Join-Path $testDirectory 'test-reaction.exe'
& $Compiler -std=c11 -Wall -Wextra -Werror `
    -I (Join-Path $projectRoot 'atividade3/questao_b/inc') `
    (Join-Path $projectRoot 'atividade3/questao_b/tests/test_reaction.c') `
    (Join-Path $projectRoot 'atividade3/questao_b/src/reaction_logic.c') `
    -o $testExecutable
if ($LASTEXITCODE -ne 0) { throw 'Falha ao compilar os testes C.' }
& $testExecutable
if ($LASTEXITCODE -ne 0) { throw 'Falha nos testes da logica de reacao.' }
