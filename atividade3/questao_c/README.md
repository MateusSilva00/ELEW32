# Questão (c) — Decodificador Morse

**Implementada e validada na placa — etapa 3.** A lógica Morse está integrada
a EXTI0, SysTick, LED e serial. Testes C aprovados e letras conferidas na
placa (ver [etapa 3](#etapa-3--integração-com-a-placa)).

Conforme o [enunciado](../../Enunciado.pdf), o botão USER permite inserir
letras em Morse: pressionamento curto representa ponto e longo representa
traço. EXTI detecta o botão, e SysTick mede as durações.

Após um segundo sem novos pressionamentos, finalizar automaticamente a letra,
decodificar e mostrar o resultado pela serial.

## Desenvolvimento por commits

1. **Base independente:** CMake, clock de 96 MHz, SysTick de 1 ms e USB CDC.
   Validar compilação, gravação DFU e identificação no terminal.
   Commit: `feat: prepara base da questao c com SysTick e USB CDC`.
2. **Lógica Morse:** definir limiar entre ponto/traço, acumular símbolos,
   finalizar letra após 1000 ms de intervalo solto, decodificar letras e
   tratar sequências inválidas. Testar limites temporais e wrap em C.
   Commit: `feat: implementa logica do decodificador Morse`.
3. **Integração:** capturar pressão/soltura de USER por EXTI0, validar debounce
   preservando timestamps e transmitir a sequência/letra no laço principal.
   Validar letras curtas e longas na placa, sem delays nas interrupções.
   Commit: `feat: integra botao e serial ao decodificador Morse`.
4. **Validação final:** testar letras sucessivas, limites, botão mantido,
   reinicialização, reconexão e entradas inválidas; documentar evidências e
   distinguir testes simulados dos físicos.
   Commit: `docs: documenta e valida a questao c`.

Uma etapa por vez, com commit feito pelo usuário antes de avançarmos.
O enunciado não fixa o limiar entre pressão curta e longa. Adotamos **300 ms**:
menos que isso é ponto; 300 ms ou mais é traço. O intervalo de **1000 ms**
entre letras é contado com o botão solto, sem encerrar durante uma pressão longa.

## Base desta etapa

Os arquivos em `src/` e `inc/` foram copiados do suporte validado da questão
(b), commit `00fff04`, removendo as chamadas ao teste de reação. Cada questão
compila seus próprios fontes; não há dependência dos fontes de (a) ou (b).
A infraestrutura CMake, o linker e as bibliotecas CMSIS/HAL/USB em `vendor/`
continuam compartilhados, com as versões já registradas.

- HSE presumido de 25 MHz, CPU de 96 MHz e clock USB de 48 MHz.
- SysTick de 1 ms, prioridade 0, com um único contador usado por HAL_GetTick.
- USB CDC em PA11/PA12, IRQ de prioridade 2, envio no `main` com buffer próprio.
- PC13 configurado como saída ativa em nível baixo, mantida apagada.
- PA0/EXTI0 ainda não configurados; pressionar USER não gera mensagem nesta base.

## Compilar e gravar

Abra a raiz `ELEW32` no VS Code. ARM GCC, CMake e Ninja devem estar no PATH.
O painel CMake Tools e o IntelliSense estão selecionados para (c). Execute
**Ctrl+Shift+B**, escolhendo **c**, ou use o terminal na raiz:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/project.ps1 -Question c -Action Build
```

ELF, HEX, BIN e MAP ficam em `out/build/questao_c/`, com nome `questao_c`.
Os builds de (a) e (b) permanecem separados.

Para gravar, mantenha o cabo USB conectado. Segure BOOT0, aperte e solte
RESET, espere meio segundo e solte BOOT0. O bootloader **0483:DF11** usa o
WinUSB e o dfu-util já preparados nas questões anteriores.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/project.ps1 -Question c -Action Flash
```

O comando compila, grava e verifica a leitura de volta. Isso substitui o
firmware anterior na placa; os projetos anteriores continuam preservados.
Após o sucesso, pressione RESET com BOOT0 solto e abra a serial:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/serial-monitor.ps1
```

O monitor identifica a COM automaticamente e ativa DTR. A saída esperada,
digitando `.-` e depois `-`, é:

```text
Questao (c): Morse A-Z.
Ponto <300 ms; traco >=300 ms; intervalo entre letras: 1 s.
Solte USER e digite.
Morse .- -> A
Morse - -> T
```

O LED de usuário acende enquanto USER está pressionado (após o debounce).
Cada letra aparece 1 segundo após a última soltura. Sem terminal aberto com
DTR, a entrada fica desativada; reabrir o terminal reapresenta a identificação.
O monitor encerra após 60 s por padrão; use `-Seconds 600` para sessões longas.

## Validação da etapa 1

Em 13/10/2026, configuração e compilação em `out/build/questao_c/` terminaram
sem avisos com ARM GNU 14.2.1. ELF, HEX, BIN e MAP foram gerados: **13568 bytes
de Flash** e **3092 bytes de RAM estática**, além da pilha em execução.
SP inicial `0x20020000` e Reset `0x080032B1` (bit Thumb ativo) foram conferidos.
Os 18 fontes compilados pertencem somente a (c) e `vendor/`.

```text
SHA-256 do BIN: 142CA86AA6F84C9B87CE61526479F20009BA70E3B7052128D9D90A4A1915B782
```

A gravação por USB DFU foi concluída e os 13568 bytes lidos de volta têm o
mesmo SHA-256 do BIN. Após RESET com BOOT0 solto, o usuário confirmou o LED
de usuário apagado. O monitor reconheceu **COM9**, recebeu as duas linhas
esperadas com DTR ativo e foi fechado normalmente após 5 segundos.
A [captura da etapa 1](docs/evidencias/etapa-1-serial.txt) foi preservada,
com as mensagens de identificação da base daquela etapa.

Não houve medição instrumental do clock nem inspeção do contador por depurador.
Na etapa 1 ainda não havia decodificação para testar.

## Etapa 2 — Lógica e testes

O [guia da lógica](docs/logica.md) descreve as regras, os limites e o contrato
de integração. O módulo em [morse_logic.c](src/morse_logic.c) decodifica A–Z,
produz um resultado por letra e distingue sequência desconhecida de excesso
de símbolos. O limiar é configurável em [morse_logic.h](inc/morse_logic.h).

Na raiz `ELEW32`:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test-morse.ps1
```

Ou execute **Questao (c): Morse Tests** no VS Code. O script usa o TCC local
em `out/tools/tcc/`; em uma máquina nova, acrescente `-Setup`. Também aceita
`-Compiler` com o caminho de outro compilador C nativo compatível. O executável
fica em `out/tests/questao_c/`.

Validação em 13/10/2026: testes C aprovados para as 26 letras, limiares
299/300/301 ms e 999/1000/1001 ms, pressão mantida, duplicatas, erros,
10 mil símbolos consecutivos, wrap e reinicialização. Consulte a
[saída dos testes](docs/evidencias/etapa-2-testes-c.txt).

O ARM GCC compilou o módulo sem avisos. Ele ainda não é chamado pelo `main`,
portanto o linker remove o código não utilizado. O BIN é idêntico à leitura
da Flash da etapa 1, com o mesmo SHA-256 e tamanho. Não houve nova gravação
ou teste físico nesta etapa; USER ainda não insere Morse na placa.

## Etapa 3 — Integração com a placa

O [guia da integração](docs/integracao.md) descreve EXTI0, SysTick, debounce,
fila de resultados, LED, mensagens e política de sessão USB. Os testes de
debounce, fila e envio foram acrescentados a `scripts/test-morse.ps1` e à
tarefa **Questao (c): Morse Tests**.

Compilação limpa sem avisos: **15672 bytes de Flash** e **3348 bytes de RAM
estática**, além da pilha em execução. SHA-256 do BIN:

```text
36471C6D3BE67C437DD12189B57CEF46385FBB1EBACE6CF73E90CF623F1AF9CD
```

Os testes C passaram: 29/30 ms, boot com USER pressionado, bouncing, pressão
mantida, borda pendente em 999/1000 ms, wrap, formatos, USB ocupado/abortado,
troca de sessão, fila cheia e 1000 ciclos. Consulte a
[saída dos testes](docs/evidencias/etapa-3-testes-c.txt).

Em 13/10/2026, a gravação DFU foi verificada: os 15672 bytes lidos de volta
têm o mesmo SHA-256 do BIN. A captura da COM9 está nas
[evidências da placa](docs/evidencias/etapa-3-placa.txt). As ações no botão e
a observação do LED foram informadas pelo usuário.

| Teste físico | Resultado |
| --- | --- |
| Identificação ao abrir o terminal | Três linhas de identificação recebidas |
| LED durante a pressão | Usuário confirmou que acende a cada pressão |
| Toque rápido | `Morse . -> E` |
| Pressão longa | `Morse - -> T` |
| `.-` | `Morse .- -> A` |
| `...` e `---` separados por 2 s | `S`, `O`/`M` conforme o agrupamento dos traços (ver abaixo) |
| USER mantido por ~5 s | Um único `T` |
| `----` com pausas curtas | `Morse ---- -> sequencia invalida` (duas vezes) |
| Cinco ou mais toques curtos | `Morse (prefixo): .... -> mais de 4 simbolos` |

Nas primeiras tentativas, alguns grupos de traços se dividiram (`--` + `-`,
`---` + `-`) e outros se uniram (`---` com `...`). Isso corresponde à regra do
enunciado: uma pausa solta de 1 s ou mais encerra a letra. Com pausas curtas
entre os traços, `----` foi reconhecido como um único grupo. Os tempos de
pressão e pausa não foram medidos, então essa explicação é inferida.

Não houve osciloscópio nem injeção física de bouncing. Debounce, borda
pendente no limite de 1 s, wrap, USB ocupado/abortado e fila cheia foram
verificados nos testes C.
