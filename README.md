# Atividade 3 — Sistemas Embarcados

## Etapa 1: ambiente Windows

Abra a pasta **ELEW32** no VS Code. As tarefas e configuracoes em `.vscode/`
na raiz usam o codigo em `template/` e geram os novos arquivos em
`out/build/windows/`, ignorado pelo Git.

O exemplo de soma, o startup, o linker script e o build Linux anterior em
`template/build/` estao preservados. As configuracoes antigas em
`template/.vscode/` pertencem ao template; utilize a pasta ELEW32 como workspace.
O firmware ainda nao esta preparado para gravacao na Black Pill: essa adaptacao
sera feita na etapa 2.

### Ferramentas

Instale ARM GCC, CMake, Ninja e OpenOCD e deixe seus executaveis no PATH.
Os pacotes correspondentes no WinGet sao:

```powershell
winget install --exact --id Kitware.CMake --source winget
winget install --exact --id Ninja-build.Ninja --source winget
winget install --exact --id Arm.GnuArmEmbeddedToolchain --source winget
winget install --exact --id xpack-dev-tools.openocd-xpack --source winget
```

Depois de instalar, reabra o VS Code e o terminal para carregar o PATH atualizado.
Confira:

```powershell
cmake --version
ninja --version
arm-none-eabi-gcc --version
arm-none-eabi-gdb --version
openocd --version
```

Instale as extensoes recomendadas pelo workspace: C/C++, CMake Tools e
Cortex-Debug. O OpenOCD e o Cortex-Debug serao usados na etapa de depuracao.

### Configurar e compilar

No PowerShell, a partir da raiz ELEW32:

```powershell
cmake -S template -B out/build/windows -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build out/build/windows
```

No VS Code, execute **Tasks: Run Task > CMake: Configure** para configurar,
ou **Ctrl+Shift+B** para configurar e compilar. O CMake verifica os compiladores
ARM gerando uma biblioteca estatica de teste, sem depender de um ambiente de
desktop. O arquivo `compile_commands.json` e gerado na pasta de build para
auxiliar as ferramentas de analise do codigo.

Os arquivos `.elf`, `.hex` e `.bin` ficam em `out/build/windows/`.
Nao reutilize `template/build/`: ele contem caminhos absolutos de outro ambiente.

### Validacao realizada nesta etapa

- CMake 4.4.4, Ninja 1.13.2, Arm GNU Toolchain 14.2.Rel1 (GCC 14.2.1)
  e xPack OpenOCD 0.12.0-7 instalados e verificados no Windows.
- GDB ARM disponivel; extensoes C/C++ e CMake Tools ja instaladas no VS Code.
  Cortex-Debug esta recomendado para a etapa 2.
- Configuracao do CMake e compilacao do exemplo original concluidas com sucesso,
  gerando ELF, HEX, BIN e `compile_commands.json` na pasta nova.
- JSON das configuracoes valido, `git diff --check` sem erros e pasta `out/`
  ignorada pelo Git. Fontes, linker script e build antigo preservados.

A compilacao apresenta avisos do template: `resultado` recebe um valor sem ser
lido, e o suporte `nosys.specs` possui stubs de `_close`, `_lseek`, `_read` e
`_write` sem implementacao de sistema operacional. Esses avisos nao impediram
o build e nao foram ocultados. A serial USB e a adaptacao do firmware serao
tratadas nas etapas seguintes; nao houve gravacao ou teste fisico nesta etapa.

### Commit desta etapa

Revise as alteracoes com `git diff` e os novos arquivos com `git status`.
A mensagem sugerida e:

```text
build: prepara ambiente Windows para a atividade 3
```
