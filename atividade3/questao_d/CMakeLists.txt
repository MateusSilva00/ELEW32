cmake_minimum_required(VERSION 3.20)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../plataforma/stm32f411/arm-gcc-toolchain.cmake")
project(questao_d C CXX ASM)

include(../../plataforma/stm32f411/firmware.cmake)
stm32f411_add_firmware(${PROJECT_NAME}
    src/main.c
    src/rhythm_logic.c
    src/rhythm_control.c
    src/rhythm_board.c
    src/rhythm_report.c
    src/clock.c
    src/timebase.c
    src/runtime.c
    src/usb_serial.c
    src/usb_descriptors.c
    src/usb_port.c
)
