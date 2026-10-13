# Dependencias do firmware

Os arquivos foram copiados sem modificacoes dos projetos oficiais abaixo.
Apenas o subconjunto necessario para STM32F411xE com GCC foi incluido;
nao e uma distribuicao completa para outros dispositivos ou compiladores.
As copias locais permitem compilar sem downloads durante o build.

| Diretorio | Origem | Versao | Licenca |
| --- | --- | --- | --- |
| CMSIS | https://github.com/ARM-software/CMSIS_5 | 5.9.0 | Apache-2.0, em CMSIS/LICENSE.txt |
| STM32F4 | https://github.com/STMicroelectronics/cmsis-device-f4 | v2.6.10 | Apache-2.0, em STM32F4/LICENSE.md |
| STM32F4_HAL | https://github.com/STMicroelectronics/stm32f4xx-hal-driver | v1.8.3 | BSD-3-Clause, em STM32F4_HAL/LICENSE.md |
| STM32_USB_Device | https://github.com/STMicroelectronics/stm32-mw-usb-device | v2.11.3 | SLA0044 Rev5, em STM32_USB_Device/LICENSE.md |

CMSIS contem `Core/Include/{core_cm4,cmsis_version,cmsis_compiler,cmsis_gcc,mpu_armv7}.h`
da pasta `CMSIS/` do repositorio Arm.
STM32F4 contem `Include/{stm32f4xx,stm32f411xe,system_stm32f4xx}.h`,
`Source/Templates/system_stm32f4xx.c` e
`Source/Templates/gcc/startup_stm32f411xe.s` do repositorio ST.

Fontes fixadas nas tags:

- https://github.com/ARM-software/CMSIS_5/tree/5.9.0
- https://github.com/STMicroelectronics/cmsis-device-f4/tree/v2.6.10

O CMake define `STM32F411xE` para selecionar o dispositivo e
`USER_VECT_TAB_ADDRESS` para que `SystemInit()` configure VTOR em `0x08000000`.
`HSE_VALUE=25000000U` informa a frequencia presumida do cristal externo.
A configuracao de PLL e SysTick pertence a aplicacao (`src/clock.c` e
`src/timebase.c`); os arquivos de terceiros continuam sem modificacoes.

Na etapa 4, HAL fornece PCD/PCDEx e o driver LL USB, com o suporte basico
de HAL/RCC. Os demais cabecalhos incluidos sao dependencias de compilacao;
somente os fontes explicitados no CMake entram no build. USB Device inclui
o nucleo (`usbd_core`, `usbd_ctlreq`, `usbd_ioreq`) e a classe CDC.
`HAL_GetTick()` consulta `millis()` da aplicacao: nao ha outro contador ativo.
Configuracoes e adaptadores estao em `template/inc` e `template/src`.

Pacotes oficiais utilizados na importacao (os arquivos extraidos foram
copiados sem alteracao; nao ha download durante a compilacao):

- [HAL v1.8.3 ZIP](https://codeload.github.com/STMicroelectronics/stm32f4xx-hal-driver/zip/refs/tags/v1.8.3)
  SHA-256: `E2EF0EAC54507FCC7BF2FBD45328AEB8C4005DA4337EE33CE717C6FFBA0B69DF`.
- [USB Device v2.11.3 ZIP](https://codeload.github.com/STMicroelectronics/stm32-mw-usb-device/zip/refs/tags/v2.11.3)
  SHA-256: `00077ED0CA0F2B506FC2A4B5D2D8237267ABDB5983BD97A799E5E3E2E9CDD108`.
