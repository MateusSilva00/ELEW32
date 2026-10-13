# Integração do decodificador Morse — etapa 3

## Fluxo na placa

1. Abrir o terminal com DTR ativo gera uma sessão. O firmware envia a
   identificação, o limiar de 300 ms, o intervalo de 1 s e `Solte USER e digite.`.
2. Após 30 ms de soltura estável, USER passa a inserir símbolos. Se a placa
   iniciar com o botão apertado, nada é registrado até a soltura.
3. EXTI0 registra cada borda. SysTick confirma 30 ms de estabilidade e entrega
   à lógica o timestamp original da borda. O LED PC13 acende enquanto a
   pressão validada dura, como retorno visual do símbolo.
4. Cada soltura vira ponto (< 300 ms) ou traço (≥ 300 ms). Após 1000 ms de
   botão solto, SysTick encerra a letra e a coloca em uma fila de 16 resultados.
5. O `main` formata e transmite cada resultado. Ele só é removido da fila
   após a conclusão USB.

Saídas possíveis:

```text
Morse .- -> A
Morse ---- -> sequencia invalida
Morse (prefixo): .... -> mais de 4 simbolos
Resultados perdidos: 2 (fila cheia)
```

## Responsabilidades

| Arquivo | Função |
| --- | --- |
| `morse_logic.c` | Ponto/traço, fim de letra e tabela A–Z (etapa 2) |
| `morse_control.c` | Debounce, soltura inicial obrigatória e fila FIFO de resultados |
| `morse_board.c` | GPIOA/SYSCFG/EXTI0/NVIC, LED e proteção do estado compartilhado |
| `timebase.c` | Único SysTick de 1 ms, compartilhado com HAL_GetTick |
| `morse_report.c` | Formatação, envio, perdas e política de sessão USB |
| `usb_serial.c` | Buffer persistente, transporte CDC e identificador de abertura do terminal |

USER é PA0 com pull-up, ativo em nível baixo. EXTI0 usa as duas bordas, porque
a duração é medida entre pressão e soltura. PR0 é reconhecido por escrita de
1 no bit, sem read-modify-write. NVIC usa prioridades 0/1/2 para
SysTick/EXTI0/USB, como em (b).

## Debounce, timestamp e concorrência

Cada borda reinicia a janela de 30 ms. Um pulso de 29 ms é rejeitado. Após um
bouncing, a duração é medida entre as bordas que iniciam os níveis estáveis,
e não entre as confirmações. Por isso, não há soma de 30 ms ao símbolo.

Enquanto há borda em validação, SysTick suspende também o fim de letra. Uma
pressão iniciada aos 999 ms após a soltura pertence à letra atual, mesmo que
seja confirmada depois de 1000 ms. SysTick lê GPIO/PR apenas para não validar
com uma borda pendente; não cria eventos por polling.

O `main` acessa o estado em seções curtas que salvam e restauram PRIMASK.
Formatação e USB ficam fora das interrupções.

## USB e sessões

- `BUSY`: o resultado permanece na fila.
- `ABORTED`: a mesma linha é repetida na mesma sessão.
- Troca de sessão (fechar/reabrir, inclusive entre dois polls): cancela a letra
  parcial, esvazia a fila e volta a exigir soltura estável.
- Sem terminal: a entrada fica desativada e o LED apagado.

A fila de 16 resultados só enche se o terminal não consumir os envios. Nesse
caso, os resultados novos são descartados, os antigos são preservados e a
quantidade perdida é informada assim que houver espaço.

## Validação desta etapa

Os testes C usam os módulos reais de lógica, debounce, fila e relatório, com
tempo, placa e transporte simulados. Cobrem 29/30 ms, boot com botão apertado,
bouncing, pressão mantida, borda pendente em 999/1000 ms, wrap, formatos de
letra/erro, USB ocupado/abortado, troca de sessão, fila cheia e 1000 ciclos.
Consulte a [saída dos testes](evidencias/etapa-3-testes-c.txt).

Esses testes não simulam o middleware ST nem as prioridades reais do NVIC.
A validação física está no [README da questão](../README.md#etapa-3--integração-com-a-placa).
