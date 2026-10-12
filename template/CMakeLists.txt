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
)

# Linker options
add_link_options(
    ${MCU_FLAGS}
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -specs=nano.specs
    -specs=nosys.specs
    "-T${CMAKE_CURRENT_SOURCE_DIR}/linker.ld"
)

# Include directories
include_directories(
    inc
)

# Source files
file(GLOB_RECURSE SOURCES "src/*.c" "src/*.s" "src/*.S")

# Executable target
add_executable(${PROJECT_NAME}.elf ${SOURCES})

# Custom post-build targets to generate .bin, .hex, and print size
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND "${CMAKE_OBJCOPY}" -O ihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.hex
    COMMAND "${CMAKE_OBJCOPY}" -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
    COMMAND "${CMAKE_SIZE}" $<TARGET_FILE:${PROJECT_NAME}.elf>
    COMMENT "Building Hex, Bin and printing size info"
    VERBATIM
)
