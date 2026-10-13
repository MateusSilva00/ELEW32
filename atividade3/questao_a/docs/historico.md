> Hist?rico das etapas anteriores ? separa??o por quest?o. Os caminhos e
> comandos abaixo registram a estrutura usada naquele momento; para executar
> o projeto atual, siga o [guia atualizado](questao-a.md) ou o
> [README principal](../../../README.md).

# Atividade 3 — Sistemas Embarcados

A questao (a) esta implementada e validada. Consulte o
[guia final de uso, funcionamento e evidencias](questao-a.md).
As secoes abaixo preservam o desenvolvimento por etapas; descricoes de
firmwares anteriores pertencem ao respectivo momento do projeto.

## Etapa 1: ambiente Windows

Abra a pasta **ELEW32** no VS Code. As tarefas e configuracoes em `.vscode/`
na raiz usam o codigo em `template/` e geram os novos arquivos em
`out/build/windows/`, ignorado pelo Git.

O firmware final da **STM32F411CEU6** registra cliques com debounce e fila, com CMSIS e
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

Na etapa 2, o programa inclui `stm32f4xx.h`, confere a inicializacao das variaveis em
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
`template/vendor/`, com licencas e origem em [vendor/README.md](../../../vendor/README.md).

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
4. No firmware atual, confirme que o LED de usuario **PC13** pisca:
   500 ms aceso e 500 ms apagado. Na etapa 2 ele permanecia aceso.
   O LED de alimentacao sozinho nao confirma a execucao do programa.

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
fica em `out/build/windows/dfu-readback.bin` e e renovada em cada verificacao.

O BIN puro pode gerar o aviso `Invalid DFU suffix signature`; o endereco e o
dispositivo sao informados explicitamente. Uma falha de gravacao ou verificacao
encerra o script com erro.

O LED acende apenas depois de confirmar `firmware_iniciado == 0` (.bss) e
`SystemCoreClock == 16000000` (.data), antes de atualizar essas variaveis.
Nao ha breakpoints ou inspecao de variaveis por USB DFU: o criterio fisico
desta etapa e a verificacao da Flash e a confirmacao visual do LED.

Nas etapas 2 e 3, o firmware ainda nao expunha uma porta COM. A partir da
etapa 4, a aplicacao expoe USB CDC. Para uma nova gravacao, repita BOOT0/RESET para usar
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

## Etapa 3: clock de 96 MHz e SysTick de 1 ms

Na etapa 3, o firmware alterna PC13 a cada 500 ms usando o contador do SysTick. Um ciclo
completo (aceso e apagado) dura aproximadamente um segundo. Ainda nao ha
USB CDC nem interrupcao do botao; a gravacao continua pelo bootloader DFU.

### Clock e cristal

Usamos **HSE de 25 MHz como hipotese**, conforme o padrao documentado pela
[WeAct](https://github.com/WeActStudio/WeActStudio.MiniSTM32F4x1).
O usuario nao conseguiu identificar a marcacao do cristal nesta placa.
O ritmo observado do LED e compativel com a hipotese, mas nao e uma medicao
precisa da frequencia. `SystemCoreClockUpdate()` calcula a frequencia a partir
dos registradores e de `HSE_VALUE`; ele nao mede o cristal.

`template/src/clock.c` configura os registradores RCC, PWR e FLASH usando
as definicoes CMSIS do dispositivo. O CMake define `HSE_VALUE=25000000U` para
que a aplicacao e o codigo da ST usem o mesmo valor.

| Elemento | Configuracao | Frequencia esperada |
| --- | --- | --- |
| Entrada do PLL | HSE / PLLM = 25 MHz / 25 | 1 MHz |
| VCO | Entrada x PLLN = 1 MHz x 192 | 192 MHz |
| SYSCLK e AHB/CPU | VCO / PLLP = 192 MHz / 2; AHB / 1 | 96 MHz |
| APB1 | HCLK / 2 | 48 MHz |
| APB2 | HCLK / 1 | 96 MHz |
| Clock para USB | VCO / PLLQ = 192 MHz / 4 | 48 MHz |

A CPU permanece no HSI enquanto o PLL e preparado. Configuramos a escala 1
do regulador com PLL desligado e 3 wait states da Flash para operacao em
3,3 V, antes de aumentar o clock. A troca para PLL ocorre depois de verificar
HSE pronto, PLL travado e regulador pronto. As esperas possuem limite de
leituras; esse limite nao representa milissegundos. Se a inicializacao falhar,
o programa permanece com o LED apagado.

Referencia dos registradores e limites: [manual ST RM0383](https://www.st.com/resource/en/reference_manual/dm00119316-stm32f411xce-advanced-armbased-32bit-mcus-stmicroelectronics.pdf),
capitulos RCC, PWR e FLASH. A sequencia de ajuste de tensao tambem pode ser
conferida em `HAL_PWREx_ControlVoltageScaling` do
[driver oficial da ST](https://github.com/STMicroelectronics/stm32f4xx-hal-driver/blob/v1.8.3/Src/stm32f4xx_hal_pwr_ex.c).
Essa referencia nao adiciona HAL ao projeto.

### Como o SysTick chega ao handler

`template/src/timebase.c` chama `SysTick_Config(SystemCoreClock / 1000U)`.
Com 96 MHz, sao **96.000 ciclos por milissegundo**. A funcao CMSIS configura
`LOAD = 95999`, limpa o contador atual e habilita o temporizador com clock
da CPU e geracao de interrupcao. Definimos prioridade 0 para essa ISR curta.
SysTick e uma excecao do nucleo; sua habilitacao esta no proprio SysTick,
sem chamar `NVIC_EnableIRQ()` como faremos para interrupcoes externas.

A entrada 15 da tabela de vetores, em `0x0800003C`, referencia
`SysTick_Handler`. O startup da ST fornece um alias `weak` para
`Default_Handler`; nossa funcao C com o mesmo nome substitui esse alias na
linkedicao. A cada interrupcao, o processador usa a tabela para executar o
handler, que apenas incrementa `milliseconds`, um `volatile uint32_t`.
Isso aplica os conceitos de tabela de vetores e ISR da Aula 06 do professor.

`millis()` retorna o contador. A leitura alinhada de 32 bits e atomica no
Cortex-M4; `volatile` garante que o compilador consulte a memoria compartilhada
com a ISR. A atualizacao do LED ocorre no `main`, usando a diferenca unsigned
`(uint32_t)(now - last_toggle) >= 500U`, sem delay dentro da interrupcao.
Entre verificacoes, `__WFI()` aguarda a proxima interrupcao.

O contador volta a zero a cada aproximadamente 49,7 dias. A subtracao unsigned
continua calculando o intervalo ao atravessar esse limite, desde que nao se
deixe passar uma volta completa sem acompanhar o tempo. A base de tempo
pressupoe que as interrupcoes nao fiquem bloqueadas por periodos prolongados:
o SysTick nao enfileira todos os ticks perdidos.

### Validacao da etapa 3

- Compilacao sem avisos, com ELF, HEX, BIN e MAP gerados: 1440 bytes de Flash
  e 12 bytes de RAM estatica, alem da pilha em execucao.
- ELF e instrucoes inspecionados: `SysTick_Handler` possui implementacao
  propria e incrementa o contador; `millis()` faz uma leitura de 32 bits.
- Firmware gravado por USB DFU; leitura de volta com SHA-256 identico ao BIN.
  Foi corrigida a renovacao do arquivo de leitura, pois dfu-util recusa
  sobrescrever a verificacao anterior.
- Apos RESET com BOOT0 solto, o usuario confirmou o LED piscando no ritmo
  esperado: aproximadamente 500 ms aceso e 500 ms apagado.
- Sem ST-Link, nao houve inspecao de registradores ou contador em tempo real.
  Nao foi feita medicao instrumental dos clocks nem teste do clock USB por
  comunicacao CDC. O valor de 48 MHz esta configurado para a etapa 4.
- O comportamento proximo ao limite de `uint32_t` ainda nao foi testado na
  placa; a validacao dos eventos nesse limite pertence a etapa 7.

Mensagem de commit sugerida:

```text
feat: configura clock e base de tempo com SysTick
```

## Etapa 4: serial USB CDC

A Black Pill agora expoe uma porta serial pelo proprio conector USB, usando
**PA11 (D-) e PA12 (D+) em AF10**, com o PHY interno do OTG FS. Nao e necessario
conectar um adaptador USB/UART. O clock de 48 MHz vem do PLL configurado na
etapa 3. O LED continua alternando a cada 500 ms.

O firmware usa USB Device **v2.11.3** (nucleo e classe CDC) e HAL STM32F4
**v1.8.3** (PCD/PCDEx, LL USB e suporte HAL/RCC), da ST. Versoes, origens,
licencas e hashes dos pacotes estao em [vendor/README.md](../../../vendor/README.md).
As dependencias estao incluidas no repositorio; o build continua sem downloads.

### Organizacao e interrupcoes

| Arquivo da aplicacao | Responsabilidade |
| --- | --- |
| `template/src/usb_descriptors.c` | Identidade CDC, strings e numero de serie derivado do UID do chip |
| `template/src/usb_port.c` | GPIO, periferico OTG FS, FIFOs, adaptacao USBD/PCD e IRQ USB |
| `template/src/usb_serial.c` | DTR, buffer de transmissao, callbacks CDC e mensagem inicial |
| `template/src/timebase.c` | Contador unico de 1 ms, compartilhado com o HAL |
| `template/inc/usbd_conf.h` e `stm32f4xx_hal_conf.h` | Configuracao local das bibliotecas |

O vetor de IRQ 67 (entrada 83, endereco `0x0800014C`) referencia
`OTG_FS_IRQHandler`, que chama o PCD. Os callbacks entregam os eventos ao
middleware USB. Essa ligacao segue o mesmo mecanismo de vetor/handler
estudado na Aula 06. USB tem prioridade 2 e SysTick prioridade 0.

`HAL_GetTick()` retorna `millis()`. Nao chamamos `HAL_Init()` e sobrescrevemos
`HAL_InitTick()` para preservar a configuracao feita pela aplicacao. Existe
somente um contador ativo, incrementado uma vez por `SysTick_Handler`.
As esperas de inicializacao do driver usam essa mesma base de tempo.

A mensagem `Sistema iniciado...` so e enviada quando o dispositivo esta
configurado pelo host e o terminal ativa **DTR**. Ela aparece uma vez por
abertura de terminal (transicao de DTR desligado para ligado). Reabrir a porta
permite ve-la novamente sem reiniciar o microcontrolador; isso nao significa
que houve novo reset da placa.

`usb_serial_poll()` roda no laco principal. Quando a classe esta ocupada,
tenta novamente em uma iteracao seguinte. `usb_serial_write()` copia ate 128
bytes para um buffer estatico proprio e retorna imediatamente. Esse buffer
so pode ser reutilizado depois de `TxState` voltar a zero, indicando conclusao
da transferencia pela classe ST (incluindo eventual pacote de tamanho zero).
O retorno `true` indica envio aceito, nao leitura confirmada pelo terminal.

Durante o acesso compartilhado ao handle e ao buffer, apenas a IRQ USB e
temporariamente mascarada; SysTick continua funcionando. Nao ha impressao,
delay de aplicacao nem espera por USB livre nas ISRs. A memoria da classe
CDC tambem e estatica, sem heap. Dados recebidos do terminal sao descartados,
e o endpoint de recepcao e rearmado; nao ha interpretador de comandos.

### Gravar e abrir o terminal

1. Entre em DFU com BOOT0/RESET e execute **DFU: Flash and Verify**.
2. Pressione RESET com BOOT0 solto. O Windows deve reconhecer a serial CDC
   (`0483:5740`), com um numero COM atribuido pelo sistema.
3. Execute **USB: Serial Monitor** nas tarefas do VS Code. O script seleciona
   automaticamente a unica placa CDC conectada e monitora por 60 segundos.
4. Confira a linha `Sistema iniciado...`. Use Ctrl+C para encerrar antes do prazo.

Tambem e possivel executar na raiz do projeto:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/serial-monitor.ps1
# Para escolher a porta e o tempo de observacao:
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/serial-monitor.ps1 -Port COM9 -Seconds 30
```

Substitua `COM9` pela porta efetiva. Feche outro terminal que esteja usando a
mesma porta antes de iniciar o monitor. A configuracao e 115200, 8N1, sem
controle de fluxo, **DTR ativo**. Em USB CDC, o baud rate e um parametro
informado ao dispositivo, sem alterar o clock USB ou configurar uma UART.

O Windows usa o driver de classe serial `usbser` para a aplicacao CDC.
O WinUSB instalado anteriormente pertence ao **STM32 BOOTLOADER (0483:DF11)**,
usado apenas durante gravacao. Nao troque o driver da porta CDC pelo Zadig.
O VID/PID `0483:5740` segue o exemplo CDC da ST para este experimento;
nao representa uma atribuicao de identificador para um produto proprio.

### Validacao da etapa 4

- Build sem avisos: ELF, HEX, BIN e MAP; 13480 bytes de Flash e 3084 bytes de
  RAM estatica, alem da pilha em execucao.
- Inspecao do ELF/BIN confirmou os handlers proprios de SysTick e OTG FS nas
  entradas corretas da tabela, `HAL_GetTick()` da aplicacao e ausencia de um
  segundo contador HAL ativo.
- Firmware gravado por DFU e verificado por leitura de volta com SHA-256 identico.
- Windows reconheceu **Dispositivo Serial USB (COM9)**, usando o driver `usbser`.
- Teste na placa com duas aberturas sucessivas da porta: nenhuma mensagem
  antes de ativar DTR; linha exata `Sistema iniciado...` ao ativar; nenhuma
  repeticao durante 600 ms de observacao; nova mensagem ao desligar/ligar DTR.
- Entrada enviada pelo terminal foi descartada sem eco. Fechar e reabrir a
  porta permitiu receber novamente a mensagem, sem reset do microcontrolador.
- `scripts/serial-monitor.ps1` testado com selecao automatica da COM9 e janela
  de 3 segundos: exibiu a mensagem corretamente e liberou a porta ao terminar.
- Os 30 arquivos adicionados de terceiros foram comparados com os pacotes
  oficiais e permaneceram identicos. PowerShell/JSON e `git diff --check` validados.
- Ainda nao houve teste de trafego continuo, desconexao fisica durante envio
  ou fila de eventos; os eventos e a validacao completa pertencem as etapas seguintes.

Mensagem de commit sugerida:

```text
feat: adiciona comunicacao serial por USB CDC
```

## Etapa 5: bordas do botao por EXTI0

O botao USER/KEY usa **PA0, ativo em nivel baixo**: solto lemos 1;
pressionado lemos 0. O projeto usa o mapeamento KEY/PA0 documentado pela
[WeAct](https://github.com/WeActStudio/WeActStudio.MiniSTM32F4x1/tree/master/Hardware).
Use o botao USER/KEY, separado dos botoes BOOT0 e RESET.

`template/src/button.c` configura GPIO, SYSCFG, EXTI e NVIC usando CMSIS e
registradores. Nenhum handler HAL de GPIO participa da deteccao do botao.
O USB da etapa 4 continua em PA11/PA12 e o LED continua piscando.

### Caminho do sinal ate o handler

1. Habilitamos os clocks de GPIOA e SYSCFG e configuramos PA0 como entrada
   com pull-up, preservando os outros pinos do mesmo porto.
2. `SYSCFG->EXTICR[0]` seleciona PA0 para a linha EXTI0. Isso escolhe qual
   porta alimenta a linha 0; nao habilita a interrupcao sozinho.
3. `EXTI->FTSR` e `EXTI->RTSR` habilitam descida e subida, respectivamente.
   `EXTI->IMR` libera a solicitacao de interrupcao dessa linha.
4. `NVIC_EnableIRQ(EXTI0_IRQn)` habilita IRQ 6. Sua entrada na tabela e a
   **22**, no endereco `0x08000058`: as 16 primeiras entradas sao do nucleo.
5. O simbolo `EXTI0_IRQHandler` substitui o alias fraco do startup. O handler
   verifica PR0 e escreve **1** nele para reconhecer/limpar a pendencia (W1C).
   Usamos atribuicao direta, evitando limpar pendencias de outras linhas
   com uma operacao `|=` nesse registrador.
6. A ISR registra nivel lido, instante retornado por `millis()` e numero de
   atendimentos. Nao imprime, nao espera pelo USB e nao executa delay.

Esse fluxo aplica GPIO/SYSCFG/EXTI, NVIC e tabela de vetores apresentados na
Aula 06 do professor. As prioridades sao SysTick=0, EXTI0=1 e USB=2, portanto
o tick pode interromper o atendimento do botao, e o botao pode interromper USB.

`button_snapshot()` copia os tres campos do ultimo registro com apenas a IRQ
EXTI0 temporariamente desabilitada. SysTick e USB continuam funcionando.
`volatile` garante acessos a memoria, mas nao tornaria sozinho a copia de
varios campos coerente entre ISR e main.

### Observar pelo USB

Como usamos apenas o cabo USB, a observacao acontece pelo terminal, sem
inspecao ao vivo no depurador. Execute **USB: Serial Monitor** e pressione e
solte USER/KEY, mantendo cada estado por cerca de dois segundos no primeiro
teste. A formatacao e o envio ficam em `button_trace_poll()`, chamado pelo main.

Exemplo ilustrativo, ainda sem debounce:

```text
Sistema iniciado...
Botao: PA0=1 (solto), irq=0, t=24 ms, delta=0
Botao: PA0=0 (pressionado), irq=1, t=2387 ms, delta=1
Botao: PA0=1 (solto), irq=4, t=4390 ms, delta=3
```

- `irq`: total de atendimentos da EXTI0 desde a inicializacao, **nao total de cliques**.
- `t`: instante do atendimento da ultima interrupcao, com resolucao de 1 ms.
  No registro inicial (`irq=0`), e o instante de inicializacao do botao.
- `delta`: atendimentos desde o ultimo registro aceito para transmissao USB.
  Valores maiores que 1 mostram que ocorreram outros atendimentos entre envios.

Ainda nao existe fila: o main observa o ultimo registro disponivel e tenta
envia-lo quando o USB estiver livre. Registros intermediarios podem ser
substituidos, inclusive enquanto o terminal estiver fechado. Ao reabrir,
a mensagem de inicio reaparece; o diagnostico do botao so reaparece se houver
registro ainda nao enviado ou uma nova interrupcao.

Uma pressao/soltura mecanica pode produzir varias bordas por bouncing. O
contador pode evidenciar essas oscilacoes, mas esta etapa nao captura todas
as bordas fisicas: EXTI tem um bit de pendencia, nao uma fila, e a ISR amostra
o nivel atual. O timestamp e do atendimento, nao de captura por hardware.
Varias interrupcoes no mesmo milissegundo tambem podem ter timestamps iguais.

O debounce de 30 ms, a contagem definitiva de pressionamentos e a fila de 32
eventos serao adicionados na etapa 6. Este diagnostico bruto e temporario.

### Validacao da etapa 5

- Compilacao sem avisos: ELF, HEX, BIN e MAP; 14192 bytes de Flash e 3100 bytes
  de RAM estatica, alem da pilha em execucao.
- ELF/BIN inspecionados: vetor 22 aponta para o handler proprio de EXTI0;
  SysTick e OTG FS continuam nos vetores corretos. A desmontagem da ISR mostra
  reconhecimento de PR0, leitura de PA0, consulta ao tick e incremento do contador.
- Firmware gravado por USB DFU e verificado por leitura de volta (SHA-256 identico).
- Na COM9, o registro inicial confirmou PA0=1 (solto), `irq=0`.
- A captura fisica registrou oito interrupcoes: quatro pares de pressao
  (PA0=0) e soltura (PA0=1), com `delta=1` em cada registro. Timestamps em ms:
  `75547/79017`, `83399/88393`, `92885/97375` e `101391/105199`.
- Nao houve registros adicionais durante os intervalos mantidos pressionados,
  de aproximadamente 3,5 a 5 segundos. O teste confirmou o mapeamento e a
  polaridade do botao na placa utilizada.
- Nao foram observadas bordas adicionais por bouncing nessa captura. Isso
  nao demonstra ausencia de bouncing fisico nem substitui o debounce da etapa 6.
- Registro local da captura: `out/button-exti-stage5.log` (ignorado pelo Git).
  Validacao por terminal USB; nao houve inspecao por depurador ou osciloscopio.

Mensagem de commit sugerida:

```text
feat: detecta bordas do botao por EXTI0
```

## Etapa 6: debounce, contagem e fila de eventos

O diagnostico bruto da etapa 5 foi substituido pela mensagem definitiva:

```text
Sistema iniciado...
Botao 1 pressionado em 2387 ms
```

### Quando um pressionamento e valido

Cada atendimento da EXTI0 registra o nivel de PA0 e o instante em milissegundos.
Cada nova borda reinicia a espera. SysTick verifica a estabilidade a cada 1 ms;
apos **30 ms sem nova borda**, confirma o estado candidato. O contador aumenta
somente ao confirmar pressionado, depois de uma soltura tambem estavel por
30 ms. Manter apertado nao repete; soltar nao gera mensagem de pressionamento.
Ao ligar a placa, exige-se primeiro uma soltura estavel, inclusive se USER/KEY
estiver pressionado durante o reset.

O timestamp do evento e **a borda que iniciou o intervalo estavel validado**,
nao o instante da confirmacao 30 ms depois. Por exemplo, se o bouncing produz
descidas em 100 e 111 ms, com uma subida entre elas, e a ultima descida fica
estavel, o timestamp sera 111 ms. Ele continua tendo resolucao de 1 ms e
representa o atendimento em software, conforme explicado na etapa 5.

`button_logic.c` contem a maquina de estados e a fila, sem registradores ou
dependencia de USB. `button.c` adapta essa logica ao hardware:

- EXTI0 reconhece PR0 e atualiza o candidato, sem imprimir nem esperar.
- SysTick incrementa o contador de tempo e chama `button_tick()`. Se ha PR0
  pendente, adia a confirmacao ate a EXTI0 atender; tambem confere o nivel fisico.
- Secoes criticas curtas preservam/restauram PRIMASK para proteger alteracoes
  compartilhadas entre EXTI0, SysTick e main. Nao ha USB, formatacao ou delay
  nessas secoes. SysTick tem prioridade maior que EXTI0, que tem prioridade
  maior que USB. O estado e acessado por funcoes com essa protecao; `volatile`
  sozinho nao seria suficiente para proteger a estrutura e a fila.

A diferenca unsigned `(uint32_t)(now - since_ms)` funciona ao atravessar o
limite do contador de milissegundos, desde que o sistema continue acompanhando
o tempo. A base SysTick e os timeouts do HAL continuam compartilhados.

### Fila e transmissao

Cada evento guarda numero e timestamp em uma **fila circular de 32 vagas**.
SysTick produz eventos; `button_report_poll()`, no main, consulta o mais antigo,
formata a mensagem e tenta enviar. USB ocupado ou terminal fechado faz o main
tentar novamente em outra iteracao, mantendo a fila.

A serial copia a mensagem para seu buffer proprio. Agora seu callback de
conclusao informa `DONE`, consumido por `usb_serial_take_result()`; somente
entao o main remove o evento da fila. Enquanto a transferencia estiver em
curso, o evento ainda ocupa uma das 32 vagas. Se a classe USB for reiniciada
antes da conclusao, informa `ABORTED` e o evento fica disponivel para nova tentativa.
O consumidor deve consultar esse resultado antes de outro `usb_serial_write()`.

Essa confirmacao e do transporte USB. Nao ha protocolo de confirmacao do
aplicativo no computador: nao se garante leitura humana, e uma interrupcao
de conexao em momento ambiguo pode resultar em repeticao na nova tentativa.
O numero do evento permite identificar essa repeticao.

Se a fila lotar, o novo pressionamento continua incrementando o contador,
mas seu evento e descartado; os 32 anteriores sao preservados. A quantidade
perdida e acumulada e informada depois de escoar os eventos pendentes:

```text
Eventos perdidos: 3 (fila cheia)
```

O aviso tambem aguarda a conclusao USB e nao apaga perdas novas ocorridas
enquanto estava em transito. O proximo evento pode ter um salto na numeracao,
correspondente aos pressionamentos descartados.

Com a alimentacao exclusivamente pelo USB, **desconectar o cabo desliga a
placa e apaga a fila em RAM**. Para acumular eventos sem consumo neste arranjo,
deixe o cabo conectado e feche o terminal/desative DTR. Os contadores e a fila
tambem reiniciam ao pressionar RESET; nao ha persistencia em Flash.

### Testes automatizados do codigo C

`tests/test_button.c` compila o mesmo `button_logic.c` e `button_report.c` do
firmware. Apenas o adaptador de hardware/USB e simulado nos testes; nao ha uma
segunda implementacao do debounce. Os testes executam no Windows e nao
substituem a verificacao de EXTI, prioridades, sinal eletrico ou USB na placa.

Preparacao opcional do compilador portatil de testes e execucao:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test-button.ps1 -Setup
# Nas proximas execucoes, tambem disponivel como tarefa Button: Tests:
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test-button.ps1
```

`-Setup` baixa o [Tiny C Compiler 0.9.27 oficial para Windows x64](https://download.savannah.gnu.org/releases/tinycc/tcc-0.9.27-win64-bin.zip)
somente se ausente, confere o SHA-256
`34A721949A2583FDFF725312DA092FA0F5F1F284B702E6F811C6954714FAABB2`
e extrai o pacote com suas licencas em `out/tools/`. Nao altera o PATH.
Tambem e possivel informar `-Compiler` com o caminho de GCC/Clang/TCC nativo.
O ARM GCC continua sendo o compilador do firmware; o TCC e usado somente nos testes.
`tests/compiler_compat.h` preserva a verificacao estatica do tamanho de buffer
no TCC antigo, que ainda nao reconhece `_Static_assert`.

### Validacao da etapa 6

- Compilacao ARM sem avisos: ELF, HEX, BIN e MAP; 14672 bytes de Flash e 3380
  bytes de RAM estatica, alem da pilha em execucao.
- Vetores de EXTI0, SysTick e OTG FS conferidos no ELF/BIN. Firmware gravado
  por DFU e comparado com a leitura de volta, com SHA-256 identico.
- Testes C aprovados: limite de 29/30 ms, pulso curto, bouncing simulado,
  timestamp preservado, botao mantido, soltura insuficiente/estavel, boot com
  botao pressionado e calculo temporal atravessando `UINT32_MAX`.
- Fila testada com 35 pressionamentos: 32 eventos preservados em ordem e 3
  perdas; mais 1000 ciclos de insercao/remocao conferiram reutilizacao circular.
- Consumidor testado com USB simulado: ocupado, transferencia abortada,
  repeticao do evento pendente, remocao somente na conclusao, mensagens exatas
  e novas perdas durante o envio de um aviso anterior.
- Teste fisico na COM9: o usuario realizou dois cliques normais e um terceiro
  mantido por aproximadamente 5 segundos, com soltura entre eles. Foram recebidos
  exatamente os eventos 1, 2 e 3, em `54698`, `55922` e `57516` ms, respectivamente.
  Nao houve repeticao durante o pressionamento mantido nem evento extra na soltura.
  Captura local em `out/button-debounce-stage6.log`, ignorada pelo Git.
- Saturacao da fila e passagem de `uint32_t` foram testadas em software; ainda
  nao foram provocadas na placa. A validacao fisica completa pertence a etapa 7.

Mensagem de commit sugerida:

```text
feat: registra pressionamentos com debounce e timestamp
```

## Etapa 7: validacao final e documentacao da questao (a)

O [guia final](questao-a.md) consolida montagem, compilacao, gravacao,
terminal e funcionamento; relaciona o codigo aos conceitos da Aula 06 e
separa verificacoes de build, testes C simulados e testes efetivos na placa.

A compilacao em pasta nova ficou identica ao firmware gravado na etapa 6.
Na placa, 35 cliques com o terminal fechado resultaram em 32 eventos guardados
e aviso de 3 perdas. O clique seguinte foi o numero 36. A reconexao fisica
do cabo USB reiniciou a placa e a contagem, e a COM9 voltou a transmitir.
As capturas e os resultados dos testes C estao em `docs/evidencias/`.

Bouncing controlado e a passagem de `uint32_t` por zero foram validados em
software, sem provocar esses cenarios fisicamente. Nao houve inspecao por
ST-Link nem medicao instrumental de clocks ou bordas.

Mensagem de commit sugerida:

```text
docs: documenta e valida a questao a
```
