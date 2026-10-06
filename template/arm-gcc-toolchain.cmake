# Toolchain File for ARM Cortex-M GCC Cross-Compilation
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Find compilers in system PATH
find_program(ARM_GCC_C arm-none-eabi-gcc)
find_program(ARM_GCC_CXX arm-none-eabi-g++)
find_program(ARM_GCC_OBJCOPY arm-none-eabi-objcopy)
find_program(ARM_GCC_OBJDUMP arm-none-eabi-objdump)
find_program(ARM_GCC_SIZE arm-none-eabi-size)

if(ARM_GCC_C)
    set(CMAKE_C_COMPILER ${ARM_GCC_C})
else()
    set(CMAKE_C_COMPILER arm-none-eabi-gcc)
endif()

if(ARM_GCC_CXX)
    set(CMAKE_CXX_COMPILER ${ARM_GCC_CXX})
else()
    set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
endif()

set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})

if(ARM_GCC_OBJCOPY)
    set(CMAKE_OBJCOPY ${ARM_GCC_OBJCOPY})
endif()

if(ARM_GCC_SIZE)
    set(CMAKE_SIZE ${ARM_GCC_SIZE})
endif()

# Skip compiler checks when cross-compiling
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
