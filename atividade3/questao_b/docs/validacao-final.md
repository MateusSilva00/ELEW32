# Questão (b) — Validação final

## Firmware e ambiente

Validação em **13/10/2026**, na Black Pill STM32F411CEU6, usando somente o
USB da placa. Firmware do commit **4aee86a**. Esta etapa consolida documentação
e evidências; o código do firmware permanece igual ao da etapa 3.

Uma compilação em `out/build/questao_b-final/`, sem reutilizar o cache anterior,
gerou ELF, HEX, BIN e MAP sem avisos com ARM GNU 14.2.1. Resultado: **15436
bytes de Flash**, **3144 bytes de RAM estática**, além da pilha em execução.
O BIN é idêntico à leitura de Flash obtida na gravação verificada da etapa 3:

```text
SHA-256: A6F11EB7C0CE481797778459E3D4011DF30A062A7BB46B2C77B57466E62BD064
```

Não foi necessário regravar para esta etapa. Os testes C foram executados
novamente e passaram, incluindo 50000 rodadas simuladas com espera entre
2000 e 5000 ms. A compilação e esses testes são verificações distintas dos
testes físicos abaixo.

## Matriz de verificações

| Verificação | Método e resultado |
| --- | --- |
| Compilação limpa | Computador: sem avisos; BIN igual à leitura anterior da Flash |
| Três tentativas válidas | Placa, etapa 3: exatamente três resultados, 27245, 2448 e 1822 ms |
| USER mantido e soltura | Placa, etapa 3: terceira pressão mantida por cerca de 5 s, conforme usuário; sem resultado extra durante a pressão/soltura |
| Tentativa antecipada | Placa, etapa 3: clique durante a espera produziu `Muito cedo!` |
| RESET com USER pressionado | Placa, etapa 4: usuário confirmou LED apagado; serial anunciou prontidão sem estímulo enquanto USER estava apertado |
| Recuperação após soltura | Placa, etapa 4: depois da soltura houve estímulo e um clique gerou resultado de 1368 ms |
| Fechamento do terminal durante o estímulo | Placa, etapa 4: usuário confirmou que o LED apagou e permaneceu apagado por pelo menos 5 s, mantendo alimentação USB |
| Reabertura sem RESET | Placa, etapa 4: COM9 recebeu nova identificação, prontidão e estímulo; nenhum clique foi solicitado nessa janela |
| Reconexão física USB | Placa, etapa 4: usuário retirou/reconectou cabo; COM9 retornou e um clique gerou resultado de 44147 ms |
| Debounce e bouncing | Teste C: limiar 29/30 ms, pulso curto rejeitado e timestamp do trecho estável preservado |
| Pressão no mesmo tick do prazo | Teste C: candidato anterior bloqueia estímulo e gera antecipação se validado |
| Estímulo atendido com atraso | Teste C: reação calculada pelo instante efetivo registrado, não pelo vencimento previsto |
| Resultado 0 ms e wrap | Teste C: igualdade temporal aceita; espera, reação, antecipação e debounce atravessam o zero de `uint32_t` |
| USB ocupado/abortado | Teste C com transporte simulado: preserva resultado enquanto ocupado e repete após aborto na mesma sessão |
| Troca de sessão e conclusão antiga | Teste C: fechar/reabrir cancela rodada; conclusão de envio anterior não inicia a próxima |
| Espera variável | Teste C: 50000 rodadas dentro dos limites inclusivos, sequência variável e reproduzível por semente |

## Evidências preservadas

- [Compilação limpa, comparação de BIN e testes C](evidencias/etapa-4-build-testes.txt).
- [Tentativas válidas, pressão mantida e antecipação — etapa 3](evidencias/etapa-3-placa.txt).
- [RESET com USER pressionado, soltura e fechamento da porta](evidencias/etapa-4-boot-user.txt).
- [Reabertura do terminal sem RESET](evidencias/etapa-4-reabertura.txt).
- [Reconexão física e nova tentativa](evidencias/etapa-4-reconexao.txt).

As capturas foram feitas pelo assistente; as ações no botão/cabo e as
observações visuais do LED foram informadas pelo usuário. Os valores de
1368 e 44147 ms confirmam o fluxo de medição após recuperação, sem constituir
uma referência calibrada de tempo de reação.

## Relação com CMSIS e interrupções

O material de referência é a **Aula 06 — CMSIS, Interrupções**, fornecida
pelo professor. Os requisitos específicos estão no [Enunciado.pdf](../../../Enunciado.pdf).

| Conceito da aula | Aplicação na questão (b) |
| --- | --- |
| CMSIS | Headers do Cortex-M4 e STM32F411 expõem registradores, máscaras, `NVIC_*`, `SysTick_Config` e intrínsecos de PRIMASK |
| Tabela de vetores | Startup inicializa memória e chama `main`; handlers C substituem aliases `weak`; VTOR aponta para `0x08000000` |
| SYSCFG e EXTI | Porta A selecionada para EXTI0, duas bordas habilitadas e pendência PR0 reconhecida por escrita de 1 |
| NVIC e prioridade | SysTick=0, EXTI0=1, USB=2; SysTick pode preemptar EXTI/USB, respeitando o mascaramento temporário por PRIMASK |
| SysTick | `LOAD=95999` com clock configurado de 96 MHz fornece tick nominal de 1 ms; compartilha tempo com HAL_GetTick |
| ISR curta | EXTI registra borda; SysTick valida debounce e aciona/desliga o LED; formatação e USB ficam no `main` |
| Dados compartilhados | Acesso do `main`/EXTI protegido por seções curtas que salvam/restauram PRIMASK; somente `volatile` não garantiria consistência |

O botão é detectado por EXTI. As leituras de GPIO/PR no SysTick servem para
impedir confirmação ou estímulo com uma borda pendente; não criam eventos
por polling. A implementação detalhada está no [guia da integração](integracao.md).

## Repetir a demonstração

Abra `ELEW32` no VS Code. Use o [README da questão](../README.md) para compilar,
gravar pelo bootloader DFU e abrir a serial USB CDC com DTR ativo.

1. Com USER solto, abra o terminal e aguarde o LED. Pressione e solte USER:
   deve aparecer um tempo em milissegundos.
2. Em outra rodada, mantenha a pressão por 5 s: deve haver só um resultado.
3. Para antecipar, conclua uma rodada, solte e espere cerca de 3 s após o LED
   apagar. Clique se o LED ainda estiver apagado: deve aparecer `Muito cedo!`.
   Pressões durante a pausa de 2 s entre rodadas apenas impedem o próximo início.
4. Feche o terminal. Reinicie segurando USER, abra a serial e mantenha o botão
   pressionado por mais de 5 s: o LED não deve iniciar o estímulo. Solte,
   aguarde o acendimento e faça um clique.
5. Durante outro estímulo, feche o terminal: o LED deve apagar. Reabra sem
   RESET: deve haver nova identificação, prontidão e espera para outro estímulo.
6. Feche o terminal, retire o cabo por 2 s e reconecte com BOOT0 e USER soltos.
   Abra a serial e confirme uma nova rodada. O cabo também fornece alimentação;
   sua retirada reinicia a placa, não apenas a comunicação.

Os testes no computador são reproduzidos por:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test-reaction.ps1
```

## Limites do que foi validado

Os tempos longos nas capturas incluem a espera pelas instruções. Não são
avaliação da rapidez humana nem calibração da precisão do relógio. HSE de
25 MHz permanece presumido; não houve medição instrumental dos clocks.

O timestamp é obtido em software, com resolução de 1 ms e latência das IRQs.
Não há captura física de borda nem observação óptica do instante do LED.
Após bouncing, registra-se o início do trecho estável validado; o resultado
não recebe 30 ms adicionais, embora só seja confirmado após o debounce.

Não houve injeção física controlada de bouncing, corrida no mesmo tick,
aborto USB no meio do pacote ou uma volta completa do contador. Esses casos
foram verificados em C com entradas simuladas. Os testes não simulam o
middleware ST ou o NVIC completo.

USB `DONE` confirma transporte, não leitura pelo terminal. A mudança de sessão
descarta a rodada. Não há armazenamento persistente, timeout de inatividade
ou suporte a rodadas com duração de 2³¹ ms ou mais (cerca de 24,9 dias).
