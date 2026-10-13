# Integração do teste de reação — etapa 3

## Fluxo na placa

1. Abrir o terminal com DTR ativo gera uma sessão. O firmware anuncia a
   questão e envia `Teste pronto. Solte USER e aguarde o LED acender.`.
2. Depois da conclusão USB do anúncio e de 30 ms de soltura estável, inicia
   a espera de 2000 a 5000 ms. A semente combina o instante de abertura do
   terminal (milissegundos e SysTick VAL) com o UID da placa. Não é uma
   fonte de aleatoriedade verdadeira, nem garante sementes sempre distintas.
3. Ao vencer a espera, SysTick confere o botão e pendências de EXTI antes
   de registrar o estímulo e acender PC13. O `main` envia a mensagem do LED.
4. EXTI0 registra a borda. SysTick confirma 30 ms de estabilidade, entrega
   o timestamp original à lógica e apaga o LED ao concluir a tentativa.
5. O `main` transmite `Tempo de reacao: N ms` ou `Muito cedo!`. O estado
   só é liberado após conclusão USB. Depois de 2 segundos, anuncia outra
   rodada; a soltura estável é novamente obrigatória.

Um pressionamento antes de a espera efetivamente começar (por exemplo,
durante o anúncio ou a pausa entre rodadas) impede o início até a soltura,
mas não produz `Muito cedo!`. A antecipação é detectada durante a espera.
O LED fica aceso aguardando a reação; não há timeout de inatividade nesta
versão. Fechar o terminal cancela o teste.

## Responsabilidades

| Arquivo | Função |
| --- | --- |
| `reaction_logic.c` | Estados, sorteio da espera e cálculo unsigned do intervalo |
| `reaction_control.c` | Debounce e exigência de soltura estável antes de iniciar/emitir estímulo |
| `reaction_board.c` | GPIOA/SYSCFG/EXTI0/NVIC, entrada por interrupção, LED e proteção do estado |
| `timebase.c` | Único SysTick de 1 ms, compartilhado com HAL_GetTick |
| `reaction_report.c` | Anúncios, resultados, pausa entre rodadas e política de sessão |
| `usb_serial.c` | Buffer persistente, transporte CDC e identificador de abertura do terminal |

PC13 já é configurado apagado no `main`. USER é PA0 com pull-up, ativo em
nível baixo. EXTI0 usa as duas bordas e reconhece PR0 por escrita de 1 no bit,
sem read-modify-write. NVIC usa prioridades 0/1/2 para SysTick/EXTI0/USB.
Os handlers substituem os aliases `weak` do startup CMSIS.

## Debounce, timestamp e concorrência

Cada borda reinicia os 30 ms. Um pulso de 29 ms é rejeitado; após bouncing,
o timestamp é o início do trecho estável validado. Portanto não se somam
30 ms ao resultado, embora o LED só apague depois da confirmação.

SysTick nunca inventa eventos de botão por polling: a consulta a GPIO/PR
apenas impede validar ou acender o LED com uma borda pendente. A entrada
vem de EXTI0. Uma pressão em validação no vencimento da espera bloqueia o
estímulo; se confirmada, é antecipação. Se rejeitada, exige 30 ms de soltura
estável antes do estímulo. Nesse caso a espera real pode ultrapassar os 5 s.

O estado compartilhado é acessado pelo `main` e EXTI em seções curtas que
salvam/restauram PRIMASK. SysTick tem a maior prioridade e não é preemptado
pelas demais IRQs. Não há envio USB, formatação ou espera em interrupções.

Timestamp e escrita do LED ocorrem no mesmo atendimento de SysTick, com
resolução de 1 ms. É medição em software: latência de interrupções, poucos
ciclos entre a conferência da entrada e o acionamento, e resolução de 1 ms
limitam a precisão. Não há captura física de borda nem medição óptica do LED.
O contrato temporal da lógica continua sendo de rodadas menores que 2³¹ ms.

## USB e resultados

O identificador de sessão muda a cada subida de DTR. Isso detecta inclusive
fechar/reabrir entre dois polls do `main`. Fechar o terminal ou perder a
configuração USB cancela a rodada e apaga o LED no próximo atendimento do
`main`. Uma conclusão de envio da sessão anterior não arma o novo teste.

O transporte copia cada mensagem para seu buffer. `BUSY` mantém o resultado;
`ABORTED` permite repetir a mensagem na mesma sessão. Uma mudança de sessão
descarta a rodada e inicia um novo anúncio. Se o resultado já ocorreu
enquanto a mensagem do estímulo ainda estava ocupada, o anúncio do LED é
entregue antes do resultado, ainda que o LED já tenha apagado.

`DONE` confirma o transporte USB, não a leitura pelo terminal. O LED é a
referência do teste; o texto pode chegar depois por latência USB/Windows.
Retirar o cabo também desliga a placa neste arranjo e apaga o estado em RAM.

## Validação desta etapa

Os testes C usam os módulos reais de lógica, debounce e envio com adaptadores
simulados de tempo, placa e transporte. Cobrem 29/30 ms, bouncing, boot com
botão pressionado, antecipação no mesmo tick do prazo, soltura, resultado de
284 ms, USB ocupado/abortado, sequência de mensagens e mudança de sessão.
Esses testes não simulam o middleware ST nem as prioridades reais do NVIC.

Compilação ARM sem avisos: 15436 bytes de Flash e 3144 bytes de RAM estática.
Vetores conferidos: SysTick `0x08000A41`, EXTI0 `0x08000609`, USB `0x08000FE1`
(endereços com bit Thumb). A gravação DFU foi verificada por leitura de volta.

```text
SHA-256: A6F11EB7C0CE481797778459E3D4011DF30A062A7BB46B2C77B57466E62BD064
```

Em 13/10/2026, a [captura da COM9](evidencias/etapa-3-placa.txt) registrou:

| Teste físico | Resultado |
| --- | --- |
| Três tentativas com LED aceso | Três resultados: 27245, 2448 e 1822 ms |
| USER mantido por cerca de 5 s na terceira tentativa | Usuário confirmou a ação; sem resultado adicional durante a pressão/soltura |
| Tentativa antecipada | Após uma rodada de preparação, clique com LED apagado produziu `Muito cedo!` |

O primeiro tempo e o tempo de 163523 ms da rodada de preparação incluem a
espera pelas instruções. Não representam um ensaio de rapidez humana nem
uma medição independente da exatidão temporal. Não houve osciloscópio ou
analisador lógico. Os casos de bouncing controlado, wrap, falha USB e corrida
no mesmo tick foram [testados em C](evidencias/etapa-3-testes-c.txt), não
injetados fisicamente na placa. A validação final ampliada fica para a etapa 4.
