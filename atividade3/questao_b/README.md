# Questão (b) — Tempo de reação

**Em desenvolvimento — etapa 1.** O projeto já inicializa clock, SysTick e
USB CDC, mantendo o LED de usuário apagado. A medição de reação e o botão
serão implementados nos próximos commits.

Conforme o [enunciado](../../Enunciado.pdf), o sistema deve anunciar que está
pronto, aguardar alguns segundos sem revelar o instante exato do estímulo e
acender o LED, informando o início pela serial. O botão USER deve ser atendido
por interrupção externa; SysTick fornece a base de tempo.

Após o pressionamento, informar o tempo de reação em milissegundos, por
exemplo `Tempo de reacao: 284 ms`. Se o botão for pressionado antes do LED,
informar `Muito cedo!`.

## Desenvolvimento por commits

1. **Base independente:** CMake, clock de 96 MHz, SysTick de 1 ms, USB CDC e
   LED inicialmente apagado. Compilar e verificar a identificação no terminal.
   Commit: `feat: prepara base da questao b com SysTick e USB CDC`.
2. **Lógica do teste:** representar os estados de espera, estímulo, resultado
   e antecipação; calcular tempos com subtração unsigned e testar os limites
   em C no computador. Definir a variação do intervalo de espera.
   Commit: `feat: implementa logica do teste de tempo de reacao`.
3. **Integração na placa:** EXTI para USER, tratamento de bouncing, instante
   do estímulo associado ao acionamento de PC13 e mensagens pelo laço principal.
   Validar resultado e `Muito cedo!`, sem bloqueios nas interrupções.
   Commit: `feat: integra botao e LED ao teste de reacao`.
4. **Validação e documentação:** testar tentativas válidas, antecipadas,
   botão mantido, reinicialização e reconexão; registrar evidências e distinguir
   testes em C dos testes físicos.
   Commit: `docs: documenta e valida a questao b`.

Uma etapa por vez: o usuário faz o commit antes de avançarmos.

## Compilar a etapa 1

Na raiz `ELEW32`, com ARM GCC, CMake e Ninja no PATH:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/project.ps1 -Question b -Action Build
```

Ou use **Ctrl+Shift+B** e selecione **b**. Os arquivos `questao_b.elf`,
`questao_b.hex`, `questao_b.bin` e `questao_b.map` ficam em `out/build/questao_b/`.
O painel CMake Tools e o IntelliSense estão selecionados para esta questão.

O startup, linker, CMSIS, HAL e middleware USB vêm da infraestrutura
compartilhada. Os adaptadores em `src/` e `inc/` foram copiados da base validada
de (a), no commit `5e67867`, para permitir evolução independente. Não há
referências aos fontes de (a) na compilação de (b).

Mantemos HSE presumido de 25 MHz, CPU de 96 MHz e USB de 48 MHz. O SysTick
atualiza somente o contador de milissegundos, também consultado por HAL_GetTick.
PA11/PA12 atendem USB; PC13 é saída ativa em nível baixo e permanece apagada.
Nesta etapa PA0/EXTI0 ainda não são configurados.

## Gravar e observar

Com o USB conectado, segure BOOT0, aperte e solte RESET, espere meio segundo
e solte BOOT0. O driver WinUSB para **STM32 BOOTLOADER (0483:DF11)** e o
dfu-util já foram preparados nas etapas de (a).

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/project.ps1 -Question b -Action Flash
```

O script compila e grava o BIN de (b), substituindo o firmware de (a) na
Flash da placa, e confere a leitura de volta. Após o sucesso, pressione RESET
com BOOT0 solto e abra o monitor:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/serial-monitor.ps1
```

Com DTR ativo, o esperado é:

```text
Questao (b): base pronta.
Etapa 1: LED apagado; medicao ainda nao habilitada.
```

O LED de alimentação permanece aceso; o LED de usuário deve ficar apagado.
Reabrir o terminal reapresenta a identificação. Ainda não há rodada nem
mensagem de tempo de reação ao apertar USER. O número da porta COM é
descoberto pelo monitor; não se presume que continuará sendo COM9.

## Validação da etapa 1

Em 13/10/2026, a configuração e compilação em uma pasta nova terminaram sem
avisos, com ARM GNU 14.2.1: **13536 bytes de Flash** e **3084 bytes de RAM
estática**, além da pilha em execução. ELF, HEX, BIN e MAP foram gerados.
A tabela de vetores contém SP inicial `0x20020000` e Reset `0x0800329D`
(bit Thumb ativo). A lista de compilação contém somente fontes de (b) e
de `vendor/`; os arquivos de (a) permanecem inalterados.

A gravação por USB DFU foi concluída e os 13536 bytes lidos de volta têm o
mesmo SHA-256 do BIN:

```text
AD9352220718A774DEC3C730ECAEEB26547EF21EF14CA78FDBFC83AF5732BF96
```

Após RESET com BOOT0 solto, o usuário confirmou o LED de usuário apagado.
O monitor reconheceu **COM9** e recebeu as duas linhas de identificação
esperadas, com DTR ativo. A [captura da etapa 1](docs/evidencias/etapa-1-serial.txt)
registra essa leitura. Não houve medição instrumental do clock nem observação
do contador SysTick por depurador; as frequências acima são as configuradas.

Os testes de lógica serão adicionados com a implementação da máquina de
estados no próximo commit. Esta etapa não valida medição de tempo de reação.
