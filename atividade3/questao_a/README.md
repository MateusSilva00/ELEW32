# Questão (a) — Contador de pressionamentos

Implementada: botão USER/KEY em PA0 por EXTI0, debounce de 30 ms com SysTick,
fila de 32 eventos e mensagens com número e timestamp pela serial USB CDC.

Consulte o [guia de uso, funcionamento e validação](docs/questao-a.md) e o
[histórico das etapas](docs/historico.md).

A partir da raiz `ELEW32`:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/project.ps1 -Question a -Action Build
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test-button.ps1
```

Os fontes e testes são deste projeto; o CMake utiliza a infraestrutura em
`plataforma/stm32f411/` e as dependências em `vendor/`, na raiz do repositório.
O firmware fica em `out/build/questao_a/questao_a.bin`.
