# Atividade 3 — Sistemas Embarcados

## Etapa 1: ambiente Windows

Abra a pasta **ELEW32** no VS Code. As tarefas e configuracoes em `.vscode/`
na raiz usam o codigo em `template/` e geram os novos arquivos em
`out/build/windows/`, ignorado pelo Git.

O firmware atual e o minimo da **STM32F411CEU6** (etapa 2), com CMSIS e
gravacao exclusivamente pelo **USB da placa, em modo DFU**. O exemplo da etapa 1 permanece no historico do
Git. O startup generico e `soma.s` continuam em `template/src/`, mas estao fora
da lista de compilacao. O build Linux anterior em `template/build/` foi preservado.
As configuracoes antigas em `template/.vscode/` pertencem ao template; utilize
a pasta ELEW32 como workspace e as configuracoes da raiz.

### Ferramentas

Instale ARM GCC, CMake e Ninja e deixe seus executaveis no PATH.
Os pacotes correspondentes no WinGet sao:

```powershell
winget install --exact --id Kitware.CMake --source winget
winget install --exact --id Ninja-build.Ninja --source winget
winget install --exact --id Arm.GnuArmEmbeddedToolchain --source winget
```

Depois de instalar, reabra o VS Code e o terminal para carregar o PATH atualizado.
Confira:

```powershell
cmake --version
ninja --version
arm-none-eabi-gcc --version
arm-none-eabi-gdb --version
```

Instale as extensoes recomendadas pelo workspace: C/C++ e CMake Tools.
OpenOCD e Cortex-Debug nao sao necessarios no fluxo USB DFU. Os arquivos antigos
de depuracao em `template/.vscode/` sao apenas parte do template preservado.

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

### Historico de validacao da etapa 1

- CMake 4.4.4, Ninja 1.13.2, Arm GNU Toolchain 14.2.Rel1 (GCC 14.2.1)
  e xPack OpenOCD 0.12.0-7 instalados e verificados no Windows.
- GDB ARM disponivel; extensoes C/C++ e CMake Tools ja instaladas no VS Code.
  O fluxo foi posteriormente adaptado para USB DFU na etapa 2.
- Configuracao do CMake e compilacao do exemplo original concluidas com sucesso,
  gerando ELF, HEX, BIN e `compile_commands.json` na pasta nova.
- JSON das configuracoes valido, `git diff --check` sem erros e pasta `out/`
  ignorada pelo Git. Fontes, linker script e build antigo preservados.

A compilacao da etapa 1 apresentou avisos do template: `resultado` recebe um valor sem ser
lido, e o suporte `nosys.specs` possui stubs de `_close`, `_lseek`, `_read` e
`_write` sem implementacao de sistema operacional. Esses avisos nao impediram
o build e nao foram ocultados. Na etapa 1 nao houve gravacao ou teste fisico.

### Commit desta etapa

Revise as alteracoes com `git diff` e os novos arquivos com `git status`.
A mensagem sugerida e:

```text
build: prepara ambiente Windows para a atividade 3
```

## Etapa 2: firmware minimo da STM32F411CEU6

O programa inclui `stm32f4xx.h`, confere a inicializacao das variaveis em
`.data` e `.bss`, atualiza `SystemCoreClock`, define `firmware_iniciado = 1`,
acende o LED de usuario em **PC13** e permanece em um laco com `__NOP()`.
Ainda nao configura SysTick, USB ou interrupcoes do botao. O clock apos reset
e o HSI interno de 16 MHz; `SystemCoreClockUpdate()` apenas consulta os
registradores e atualiza a variavel, sem mudar o clock.

### O caminho do reset ate main

1. A tabela de vetores em `0x08000000` fornece o topo da pilha (`0x20020000`)
   e o endereco Thumb de `Reset_Handler`.
2. O startup oficial da ST chama `SystemInit()`, que habilita o acesso a FPU
   e, com `USER_VECT_TAB_ADDRESS`, configura VTOR em `0x08000000`.
3. Copia `.data` da Flash para a RAM: por exemplo, `SystemCoreClock = 16000000`.
4. Zera `.bss`: por exemplo, `firmware_iniciado = 0`.
5. Inicializa a biblioteca C com `__libc_init_array()` e chama `main()`.

O linker descreve 512 KB de Flash e 128 KB de RAM, fornece os limites das
secoes ao startup e verifica uma reserva minima de 2 KB para a pilha.
A tabela completa inclui SysTick, EXTI0 e USB, preparados para as proximas
etapas. Os handlers ainda nao implementados apontam para `Default_Handler`.

O CMake usa uma lista explicita de fontes para evitar dois startups. A opcao
`-nostartfiles` seleciona nossa inicializacao; `runtime.c` fornece o hook
vazio `_init` esperado pela newlib. As bibliotecas continuam disponiveis.
O arquivo `.map` gerado junto do ELF permite conferir as secoes e os simbolos.

CMSIS-Core **5.9.0** e CMSIS Device STM32F4 **v2.6.10** estao incluidos em
`template/vendor/`, com licencas e origem em [vendor/README.md](template/vendor/README.md).

### Preparar a gravacao USB

Execute **Tasks: Run Task > DFU: Setup** uma vez. O script baixa o
[dfu-util 0.11 oficial](https://dfu-util.sourceforge.net/releases/), verifica
o SHA-256 do pacote e extrai os executaveis e licencas em `out/tools/`.
O CMake faz a extracao do formato XZ; nao e necessario alterar o PATH.

Com um cabo USB de dados conectado, entre no bootloader conforme a
[documentacao da WeAct](https://github.com/WeActStudio/WeActStudio.MiniSTM32F4x1#how-to-enter-isp-mode):

1. Mantenha **BOOT0** pressionado.
2. Aperte e solte **RESET/NRST**, mantendo BOOT0 pressionado.
3. Espere cerca de meio segundo e solte BOOT0.

O Windows deve mostrar **STM32 BOOTLOADER**, USB ID **0483:DF11**.
Se aparecer erro 28 no Gerenciador de Dispositivos ou
`LIBUSB_ERROR_NOT_SUPPORTED` no dfu-util, falta o driver compativel.
Use o [Zadig oficial](https://zadig.akeo.ie/), selecione exatamente
**STM32 BOOTLOADER (0483:DF11)** e instale **WinUSB**. Se necessario, use
**Options > List All Devices**. Essa configuracao e feita uma vez no Windows.

### Gravar, verificar e executar

1. Abra **ELEW32** no VS Code e execute **DFU: List**; a listagem deve mostrar
   a Flash interna do dispositivo em modo DFU.
2. Execute **DFU: Flash and Verify**. A tarefa compila, grava a Flash interna
   em `0x08000000` e compara o SHA-256 da leitura de volta com o BIN compilado.
3. Apos a mensagem de sucesso, pressione **RESET** com **BOOT0 solto**.
4. Confirme que o LED de usuario **PC13** permanece aceso. O LED de alimentacao
   sozinho nao confirma a execucao do programa.

Os mesmos comandos podem ser executados no PowerShell, na raiz do projeto:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/setup-dfu.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/dfu.ps1 -Action List
cmake --build out/build/windows
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/dfu.ps1 -Action Flash
```

O script restringe a gravacao ao dispositivo DFU `0483:DF11`, alternativa 0
(Flash interna), e verifica os vetores do BIN antes de gravar. O dfu-util recusa
uma selecao ambigua se houver mais de uma placa compativel; conecte uma por vez.
Nao altera option bytes nem solicita apagamento total. A leitura de verificacao
fica em `out/build/windows/dfu-readback.bin`.

O BIN puro pode gerar o aviso `Invalid DFU suffix signature`; o endereco e o
dispositivo sao informados explicitamente. Uma falha de gravacao ou verificacao
encerra o script com erro.

O LED acende apenas depois de confirmar `firmware_iniciado == 0` (.bss) e
`SystemCoreClock == 16000000` (.data), antes de atualizar essas variaveis.
Nao ha breakpoints ou inspecao de variaveis por USB DFU: o criterio fisico
desta etapa e a verificacao da Flash e a confirmacao visual do LED.

Depois do reset, este firmware ainda nao expoe uma porta COM; USB CDC entra
na etapa 4. Para uma nova gravacao, repita a sequencia BOOT0/RESET para usar
o bootloader de fabrica, que reside na memoria de sistema e permanece disponivel.

### Validacao da etapa 2

- Compilacao concluida sem avisos: ELF, HEX, BIN e MAP gerados.
- Flash ocupada: 896 bytes; RAM estatica: 8 bytes, alem da pilha em execucao.
- ELF inspecionado: vetores em `0x08000000`, pilha inicial em `0x20020000`,
  `.data` com origem na Flash e destino na RAM, `.bss` sem conteudo de carga.
- dfu-util 0.11 preparado; scripts PowerShell e configuracoes JSON validados.
- Driver WinUSB instalado e placa reconhecida como STM32 BOOTLOADER (`0483:DF11`).
- Firmware de 896 bytes gravado em `0x08000000` por USB DFU. A leitura de volta
  da Flash teve SHA-256 identico ao BIN compilado.
- Apos RESET com BOOT0 solto, o usuario confirmou o LED de PC13 aceso
  continuamente: a aplicacao executou e passou pelas checagens de `.data` e `.bss`.

Mensagem de commit sugerida:

```text
feat: adiciona firmware minimo para a Black Pill F411
```
