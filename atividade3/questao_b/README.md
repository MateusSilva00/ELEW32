# Questão (b) — Tempo de reação

**Em desenvolvimento — etapa 2.** A lógica do teste de reação está implementada
e testada em C no computador. A integração com botão, LED e mensagens será
feita na etapa 3. Por enquanto, o firmware executa a base da etapa 1: clock,
SysTick e USB CDC, mantendo o LED de usuário apagado.

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

A etapa 1 não valida medição de tempo de reação.

## Etapa 2 — Lógica do teste

O módulo [reaction_logic.c](src/reaction_logic.c), com interface em
[reaction_logic.h](inc/reaction_logic.h), representa a rodada sem acessar
registradores ou USB. Ele escolhe uma espera entre **2000 e 5000 ms**, registra
o instante efetivo do estímulo e classifica a borda como antecipada ou como
reação válida. O resultado permanece disponível até liberação explícita.

As transições, escolhas de implementação e cuidados para a próxima etapa
estão no [guia da lógica](docs/logica.md).

Na raiz do repositório:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test-reaction.ps1
```

Ou execute **Questao (b): Reaction Tests** no VS Code. O script usa o TCC já
instalado em `out/tools/tcc/`; em uma máquina nova, acrescente `-Setup`.
O executável dos testes fica em `out/tests/questao_b/`. Pode-se fornecer
`-Compiler` com o caminho de outro compilador C nativo compatível.

Em 13/10/2026, passaram os testes de prazo, estímulo atendido com atraso,
reação de 284 ms e de 0 ms, antecipação, entrega atrasada de borda, eventos
anteriores à rodada, resultado retido, cancelamento e passagem de `uint32_t`
por zero. Foram executadas também 50000 rodadas, verificando os limites de
2000 e 5000 ms e a variação/reprodutibilidade da sequência. A saída está
registrada em [evidências dos testes C](docs/evidencias/etapa-2-testes-c.txt).

O módulo também foi compilado pelo ARM GCC, sem avisos. Ainda não é chamado
pelo `main`, portanto o linker remove suas funções não utilizadas. O BIN
permanece idêntico à leitura da Flash da etapa 1, com o SHA-256 registrado
acima e os mesmos tamanhos de Flash/RAM. Não houve nova gravação ou teste
físico nesta etapa; o terminal continua mostrando a identificação da base.
