# Integração da sequência rítmica

## Fluxo na placa

1. Abrir o terminal com DTR ativo gera uma sessão e sorteia a sequência.
   O firmware envia a identificação e as regras de classificação.
2. O `main` anuncia o nível e a sequência, e depois quantos cliques são
   esperados. A demonstração só começa após a conclusão USB dessa instrução.
3. Após 1 s, SysTick toca a sequência no LED PC13: uma batida de 100 ms por
   clique, separadas por 300 ms (curto) ou 900 ms (longo).
4. O usuário repete o ritmo com USER. EXTI0 registra as bordas; SysTick
   confirma 30 ms de estabilidade e entrega à lógica o instante original da
   pressão. Fora da demonstração, o LED acompanha a pressão validada.
5. O `main` transmite cada intervalo e, no último ou no prazo, o resultado.
   Após a conclusão do envio, aguarda 1,5 s e anuncia a próxima rodada: mais
   longa se a tentativa foi correta, ou a mesma se foi incorreta.

Exemplo de sessão (os valores medidos são ilustrativos):

```text
Questao (d): sequencia ritmica.
Curto <600 ms; longo >=600 ms; 2 s sem clique encerra a tentativa.
Nivel 1 (3 trechos): curto longo curto
Observe o LED; depois repita com 4 cliques.
Intervalo 1/3: 320 ms -> curto (ok)
Intervalo 2/3: 880 ms -> longo (ok)
Intervalo 3/3: 650 ms -> longo (esperado curto)
Tentativa incorreta. Repita a mesma sequencia.
Nivel 1 (3 trechos): curto longo curto
Observe o LED; depois repita com 4 cliques.
Intervalo 1/3: 297 ms -> curto (ok)
Intervalo 2/3: 912 ms -> longo (ok)
Intervalo 3/3: 305 ms -> curto (ok)
Tentativa correta! Proximo nivel com mais um trecho.
Nivel 2 (4 trechos): curto longo curto longo
```

Se passarem 2 s sem clique depois do primeiro:

```text
Sem clique por 2 s: 1/3 intervalos.
Tentativa incorreta. Repita a mesma sequencia.
```

## Responsabilidades

| Arquivo | Função |
| --- | --- |
| `rhythm_logic.c` | Sorteio, demonstração, classificação, prazo e níveis |
| `rhythm_control.c` | Debounce, soltura inicial obrigatória, LED de retorno e fila de eventos |
| `rhythm_board.c` | GPIOA/SYSCFG/EXTI0/NVIC, LED, semente e proteção do estado compartilhado |
| `timebase.c` | Único SysTick de 1 ms, compartilhado com HAL_GetTick |
| `rhythm_report.c` | Anúncios, intervalos, resultados, pausa e política de sessão USB |
| `usb_serial.c` | Buffer persistente, transporte CDC e identificador de abertura do terminal |

A base (clock de 96 MHz, SysTick, USB CDC) foi copiada da questão (c), sem
dependência de seus fontes. USER é PA0 com pull-up, ativo em nível baixo.
EXTI0 usa as duas bordas para que o debounce conheça a soltura, mas só a
pressão é clique. NVIC usa prioridades 0/1/2 para SysTick/EXTI0/USB.

## Debounce, medição e concorrência

Cada borda reinicia a janela de 30 ms. Pulsos menores são rejeitados. Após um
bouncing, o intervalo usa o instante da borda que iniciou a pressão estável,
e não o da confirmação. Assim, não se somam 30 ms ao intervalo.

Com uma borda em validação, SysTick suspende o prazo de 2 s. A leitura de
GPIO/PR no SysTick apenas impede a confirmação com borda pendente; não cria
cliques por polling. O LED da demonstração continua sendo atualizado.

O `main` acessa o estado em seções curtas que salvam e restauram PRIMASK.
Formatação e USB ficam fora das interrupções. A resolução é de 1 ms, mais a
latência de interrupção; não há captura física de borda.

## USB e sessões

- `BUSY`: anúncio ou evento aguardam; nada é perdido.
- `ABORTED`: a mesma linha é repetida na mesma sessão.
- Troca de sessão, inclusive fechar/reabrir entre dois polls: nova semente e
  nova sequência de 3 trechos. A rodada, a fila e o LED são reiniciados.
- Sem terminal: entrada desativada e LED apagado.

A maior linha (12 trechos ou último intervalo com resultado) tem 96 bytes,
abaixo do buffer USB de 128 bytes, conforme verificado nos testes C.
