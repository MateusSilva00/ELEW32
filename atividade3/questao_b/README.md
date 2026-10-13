# Questão (b) — Tempo de reação

**A implementar.** Esta pasta ainda não contém firmware compilável.

Conforme o [enunciado](../../Enunciado.pdf), o sistema deve anunciar que está
pronto, aguardar alguns segundos sem revelar o instante exato do estímulo e
acender o LED, informando o início pela serial. O botão USER deve ser atendido
por interrupção externa; SysTick fornece a base de tempo.

Após o pressionamento, informar o tempo de reação em milissegundos, por
exemplo `Tempo de reacao: 284 ms`. Se o botão for pressionado antes do LED,
informar `Muito cedo!`.

O desenvolvimento seguirá em etapas com validação e um commit por etapa.
Os fontes, testes e documentação ficarão aqui, usando a plataforma compartilhada.
