# Atividade 3 — Sistemas Embarcados

Black Pill STM32F411CEU6, botão USER (PA0) por interrupção externa (EXTI0),
SysTick de 1 ms como base de tempo, LED PC13 e serial pelo USB da própria
placa (USB CDC, 115200 8N1). Cada questão é um firmware independente.

| Pasta | Questão | Arquivos principais |
| --- | --- | --- |
| `questao_a` | Contador de pressionamentos com debounce | `button*.c` |
| `questao_b` | Tempo de reação | `reaction_*.c` |
| `questao_c` | Decodificador de código Morse | `morse_*.c` |
| `questao_d` | Sequência rítmica | `rhythm_*.c` |

Em cada pasta, `src/main.c` inicializa o sistema. Os arquivos `clock.c`,
`timebase.c` e `usb_*.c` são o suporte comum: clock de 96 MHz, SysTick e USB.

Para compilar são necessários o ARM GCC e as bibliotecas da ST
(CMSIS, STM32F4 HAL e STM32 USB Device), que não estão incluídas.