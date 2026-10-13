# Dependencias do firmware

Os arquivos foram copiados sem modificacoes dos projetos oficiais abaixo.
Apenas o subconjunto necessario para STM32F411xE com GCC foi incluido;
nao e uma distribuicao completa para outros dispositivos ou compiladores.
As copias locais permitem compilar sem downloads durante o build.

| Diretorio | Origem | Versao | Licenca |
| --- | --- | --- | --- |
| CMSIS | https://github.com/ARM-software/CMSIS_5 | 5.9.0 | Apache-2.0, em CMSIS/LICENSE.txt |
| STM32F4 | https://github.com/STMicroelectronics/cmsis-device-f4 | v2.6.10 | Apache-2.0, em STM32F4/LICENSE.md |

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
Nao ha HAL nem configuracao de PLL/SysTick nesta etapa.
