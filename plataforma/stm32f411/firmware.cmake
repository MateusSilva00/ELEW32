# Suporte comum para projetos da Black Pill STM32F411 com USB CDC.
function(stm32f411_add_firmware name)
    set(platform_dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}")
    get_filename_component(vendor_dir "${platform_dir}/../../vendor" ABSOLUTE)
    # Microcontroller Architecture flags (CMake list separated by newlines/spaces)
    set(MCU_FLAGS
        -mcpu=cortex-m4
        -mthumb
        -mfloat-abi=hard
        -mfpu=fpv4-sp-d16
    )

    # Fontes da questao seguidos pelo suporte comum CMSIS, HAL e USB CDC.
    add_executable(${name}.elf
        ${ARGN}
        ${vendor_dir}/STM32F4_HAL/Src/stm32f4xx_hal.c
        ${vendor_dir}/STM32F4_HAL/Src/stm32f4xx_hal_rcc.c
        ${vendor_dir}/STM32F4_HAL/Src/stm32f4xx_hal_pcd.c
        ${vendor_dir}/STM32F4_HAL/Src/stm32f4xx_hal_pcd_ex.c
        ${vendor_dir}/STM32F4_HAL/Src/stm32f4xx_ll_usb.c
        ${vendor_dir}/STM32_USB_Device/Core/Src/usbd_core.c
        ${vendor_dir}/STM32_USB_Device/Core/Src/usbd_ctlreq.c
        ${vendor_dir}/STM32_USB_Device/Core/Src/usbd_ioreq.c
        ${vendor_dir}/STM32_USB_Device/Class/CDC/Src/usbd_cdc.c
        ${vendor_dir}/STM32F4/Source/Templates/system_stm32f4xx.c
        ${vendor_dir}/STM32F4/Source/Templates/gcc/startup_stm32f411xe.s
    )

    # Compiler options
    target_compile_options(${name}.elf PRIVATE
        ${MCU_FLAGS}
        -fdata-sections
        -ffunction-sections
        -Wall
        -g3
        $<$<CONFIG:Debug>:-Og>
    )

    # Linker options
    target_link_options(${name}.elf PRIVATE
        ${MCU_FLAGS}
        -nostartfiles
        -Wl,--gc-sections
        -Wl,--print-memory-usage
        "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${name}.map"
        -specs=nano.specs
        -specs=nosys.specs
        "-T${platform_dir}/linker.ld"
    )


    target_compile_features(${name}.elf PRIVATE c_std_11)
    target_compile_definitions(${name}.elf PRIVATE
        STM32F411xE
        USER_VECT_TAB_ADDRESS
        HSE_VALUE=25000000U
        USE_HAL_DRIVER
    )
    target_include_directories(${name}.elf PRIVATE inc)
    target_include_directories(${name}.elf SYSTEM PRIVATE
        ${vendor_dir}/CMSIS/Core/Include
        ${vendor_dir}/STM32F4/Include
        ${vendor_dir}/STM32F4_HAL/Inc
        ${vendor_dir}/STM32_USB_Device/Core/Inc
        ${vendor_dir}/STM32_USB_Device/Class/CDC/Inc
    )
    # Alteracoes no mapa de memoria tambem precisam disparar a linkedicao.
    set_property(TARGET ${name}.elf APPEND PROPERTY LINK_DEPENDS
        "${platform_dir}/linker.ld"
    )

    # Custom post-build targets to generate .bin, .hex, and print size
    add_custom_command(TARGET ${name}.elf POST_BUILD
        COMMAND "${CMAKE_OBJCOPY}" -O ihex $<TARGET_FILE:${name}.elf> ${name}.hex
        COMMAND "${CMAKE_OBJCOPY}" -O binary $<TARGET_FILE:${name}.elf> ${name}.bin
        COMMAND "${CMAKE_SIZE}" $<TARGET_FILE:${name}.elf>
        COMMENT "Building Hex, Bin and printing size info"
        VERBATIM
    )

endfunction()
