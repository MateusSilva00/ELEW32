cmake_minimum_required(VERSION 3.20)

# Set toolchain before project definition
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/arm-gcc-toolchain.cmake")

project(cortex_m_firmware C CXX ASM)

# Microcontroller Architecture flags (CMake list separated by newlines/spaces)
set(MCU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
)

# Compiler options
add_compile_options(
    ${MCU_FLAGS}
    -fdata-sections
    -ffunction-sections
    -Wall
    -g3
    $<$<CONFIG:Debug>:-Og>
)

# Linker options
add_link_options(
    ${MCU_FLAGS}
    -nostartfiles
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map"
    -specs=nano.specs
    -specs=nosys.specs
    "-T${CMAKE_CURRENT_SOURCE_DIR}/linker.ld"
)

# Lista explicita: o startup generico e soma.s do template nao entram no firmware.
add_executable(${PROJECT_NAME}.elf
    src/main.c
    src/clock.c
    src/timebase.c
    src/button.c
    src/button_logic.c
    src/button_report.c
    src/runtime.c
    src/usb_serial.c
    src/usb_descriptors.c
    src/usb_port.c
    vendor/STM32F4_HAL/Src/stm32f4xx_hal.c
    vendor/STM32F4_HAL/Src/stm32f4xx_hal_rcc.c
    vendor/STM32F4_HAL/Src/stm32f4xx_hal_pcd.c
    vendor/STM32F4_HAL/Src/stm32f4xx_hal_pcd_ex.c
    vendor/STM32F4_HAL/Src/stm32f4xx_ll_usb.c
    vendor/STM32_USB_Device/Core/Src/usbd_core.c
    vendor/STM32_USB_Device/Core/Src/usbd_ctlreq.c
    vendor/STM32_USB_Device/Core/Src/usbd_ioreq.c
    vendor/STM32_USB_Device/Class/CDC/Src/usbd_cdc.c
    vendor/STM32F4/Source/Templates/system_stm32f4xx.c
    vendor/STM32F4/Source/Templates/gcc/startup_stm32f411xe.s
)

target_compile_features(${PROJECT_NAME}.elf PRIVATE c_std_11)
target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
    STM32F411xE
    USER_VECT_TAB_ADDRESS
    HSE_VALUE=25000000U
    USE_HAL_DRIVER
)
target_include_directories(${PROJECT_NAME}.elf PRIVATE inc)
target_include_directories(${PROJECT_NAME}.elf SYSTEM PRIVATE
    vendor/CMSIS/Core/Include
    vendor/STM32F4/Include
    vendor/STM32F4_HAL/Inc
    vendor/STM32_USB_Device/Core/Inc
    vendor/STM32_USB_Device/Class/CDC/Inc
)
# Alteracoes no mapa de memoria tambem precisam disparar a linkedicao.
set_property(TARGET ${PROJECT_NAME}.elf APPEND PROPERTY LINK_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/linker.ld"
)

# Custom post-build targets to generate .bin, .hex, and print size
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND "${CMAKE_OBJCOPY}" -O ihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.hex
    COMMAND "${CMAKE_OBJCOPY}" -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
    COMMAND "${CMAKE_SIZE}" $<TARGET_FILE:${PROJECT_NAME}.elf>
    COMMENT "Building Hex, Bin and printing size info"
    VERBATIM
)
