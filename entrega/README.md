# Atividade 3 — Sistemas Embarcados

Black Pill STM32F411CE. Cada pasta é um projeto PlatformIO independente,
baseado no exemplo **BlackPill_F411CE - USB CDC**: CMSIS, TinyUSB e serial
pelo USB da própria placa. O botão USER (PA0) é tratado por interrupção
externa (EXTI0), e o SysTick de 1 ms mede os tempos.

| Pasta | Questão |
| --- | --- |
| `questao_a` | Contador de pressionamentos com debounce |
| `questao_b` | Tempo de reação |
| `questao_c` | Decodificador de código Morse |
| `questao_d` | Sequência rítmica |

A aplicação de cada questão está em `src/main.c`. Os arquivos `usb_serial.c`,
`usb_descriptors.c`, `tusb_config.h` e `platformio.ini` vêm do exemplo do
professor. No `usb_serial.c` foram acrescentadas apenas `millis()`, que lê o
contador do SysTick, e `usb_serial_task()`, além do LED apagado ao final da
inicialização.

## Uso

1. Abra a pasta da questão no VS Code com a extensão PlatformIO e compile (Build).
2. Coloque a placa em DFU (segure BOOT0, aperte RESET, solte BOOT0) e use Upload.
3. Pressione RESET e abra o monitor serial (115200). O programa começa quando o
   terminal é aberto.
