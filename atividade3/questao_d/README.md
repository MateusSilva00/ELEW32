# Questão (d) — Sequência rítmica

**Implementada e validada na placa.** Firmware compilado sem avisos, testes C
aprovados e oito níveis jogados na placa (ver [Validação](#validação)).

Conforme o [enunciado](../../Enunciado.pdf), o usuário deve reproduzir uma
sequência rítmica com o botão USER. Ela começa com três trechos, curtos ou
longos, e ganha um trecho a cada sequência correta. Cada pressionamento usa
EXTI, e o SysTick mede os intervalos entre cliques. A serial apresenta os
intervalos medidos, sua classificação e se a tentativa foi correta.

## Como funciona

O LED da placa toca a sequência: uma batida por clique, com 300 ms entre
batidas para um trecho curto e 900 ms para um longo. Depois, o usuário repete
o ritmo no USER. Um intervalo abaixo de 600 ms é curto; a partir disso, longo.
Passar 2 s sem clicar, depois do primeiro clique, encerra a tentativa.

Um acerto acrescenta um trecho; um erro repete a mesma sequência. Ao acertar
12 trechos, uma nova sequência de 3 é sorteada. Esses valores não constam do
enunciado: são escolhas descritas no [guia da lógica](docs/logica.md). O
[guia da integração](docs/integracao.md) descreve EXTI0, SysTick, debounce,
LED, mensagens e o comportamento do USB.

## Estrutura

A base (CMake, clock de 96 MHz, SysTick de 1 ms, USB CDC) foi copiada da
questão (c), sem dependência de seus fontes. Infraestrutura CMake, linker e
bibliotecas em `vendor/` continuam compartilhados.

| Arquivo | Função |
| --- | --- |
| [rhythm_logic.c](src/rhythm_logic.c) | Lógica pura: sorteio, demonstração, classificação, prazo e níveis |
| [rhythm_control.c](src/rhythm_control.c) | Debounce de 30 ms e fila de eventos |
| [rhythm_board.c](src/rhythm_board.c) | EXTI0, NVIC, LED e seções críticas |
| [rhythm_report.c](src/rhythm_report.c) | Mensagens pela serial, no `main` |

## Compilar e testar

Na raiz `ELEW32`, com ARM GCC, CMake e Ninja no PATH:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/project.ps1 -Question d -Action Build
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test-rhythm.ps1
```

Ou use **Ctrl+Shift+B** escolhendo **d**, e a tarefa **Questao (d): Rhythm
Tests**. ELF, HEX, BIN e MAP ficam em `out/build/questao_d/`; o executável
dos testes, em `out/tests/questao_d/`. Em uma máquina nova, acrescente
`-Setup` ao script de testes para instalar o TCC local.

O painel CMake Tools e o IntelliSense continuam configurados para (c). Para
navegar em (d) com IntelliSense, ajuste `cmake.sourceDirectory` e
`cmake.buildDirectory` em `.vscode/settings.json`.

## Gravar e observar

Com o USB conectado, segure BOOT0, aperte e solte RESET, espere meio segundo
e solte BOOT0. Então:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/project.ps1 -Question d -Action Flash
```

O comando compila, grava e confere a leitura de volta, substituindo o
firmware anterior. Pressione RESET com BOOT0 solto e abra a serial:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/serial-monitor.ps1 -Seconds 600
```

```text
Questao (d): sequencia ritmica.
Curto <600 ms; longo >=600 ms; 2 s sem clique encerra a tentativa.
Nivel 1 (3 trechos): curto longo curto
Observe o LED; depois repita com 4 cliques.
Intervalo 1/3: 297 ms -> curto (ok)
Intervalo 2/3: 912 ms -> longo (ok)
Intervalo 3/3: 305 ms -> curto (ok)
Tentativa correta! Proximo nivel com mais um trecho.
Nivel 2 (4 trechos): curto longo curto longo
```

A sequência é sorteada a cada abertura do terminal, e os tempos acima são
ilustrativos. Fechar o terminal cancela a rodada e apaga o LED.

## Validação

Em 13/10/2026, a compilação limpa terminou sem avisos com ARM GNU 14.2.1:
**16528 bytes de Flash** e **3308 bytes de RAM estática**, além da pilha.
SHA-256 do BIN:

```text
8CC64008EDC049E8C764A556C087053A2F696CF1D0FFB579892AB602615EBE98
```

Os testes C compilam os mesmos `rhythm_logic.c`, `rhythm_control.c` e
`rhythm_report.c` do firmware, com tempo, placa e USB simulados. Cobrem:

- 20000 sementes iniciais, sempre com trechos curtos e longos;
- tempos de LED da demonstração;
- limiares 599/600 ms e prazo 1999/2000 ms, inclusive com a borda pendente;
- níveis de 3 a 12 trechos, preservando o prefixo, e repetição após erro;
- wrap do contador e debounce de 29/30 ms com bouncing;
- mensagens, pausa entre rodadas, USB ocupado/abortado e troca de sessão.

Consulte a [saída dos testes](docs/evidencias/testes-c.txt). Eles não
simulam o middleware ST nem as prioridades reais do NVIC.

### Na placa

Em 13/10/2026, a gravação DFU foi verificada: os 16528 bytes lidos de volta
têm o mesmo SHA-256 do BIN. A captura da COM9 está nas
[evidências da placa](docs/evidencias/placa.txt). As ações no botão foram
feitas pelo usuário.

| Requisito / caso | Resultado na placa |
| --- | --- |
| Sequência inicial de 3 trechos, curtos e longos | `curto longo longo` |
| Intervalos medidos e classificados | Ex.: `Intervalo 2/3: 1069 ms -> longo (ok)` |
| Tentativa correta aumenta a dificuldade | Do nível 1 (3 trechos) ao nível 8 (10), sempre preservando os trechos anteriores |
| Tentativa incorreta | `569 ms -> curto (esperado longo)` + `Tentativa incorreta`; mesma sequência repetida |
| Vários erros na mesma tentativa | Nível 5: três intervalos com `(esperado ...)` reportados |
| Prazo de 2 s | `Sem clique por 2 s: 0/3`, `6/7`, `5/7` e `0/8 intervalos` |
| Repetição da demonstração | Um clique seguido de espera encerra a tentativa e toca a sequência de novo |

Na placa não foram verificados: os cliques durante a demonstração, o
fechamento e a reabertura do terminal, nem o nível máximo de 12 trechos.
Esses casos, o debounce, os limiares exatos e o wrap foram cobertos pelos
testes C.
