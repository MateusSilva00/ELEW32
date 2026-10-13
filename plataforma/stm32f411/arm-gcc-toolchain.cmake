# Toolchain File for ARM Cortex-M GCC Cross-Compilation
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Falhar na configuracao se alguma ferramenta ARM estiver ausente do PATH.
find_program(ARM_GCC_C arm-none-eabi-gcc REQUIRED)
find_program(ARM_GCC_CXX arm-none-eabi-g++ REQUIRED)
find_program(ARM_GCC_OBJCOPY arm-none-eabi-objcopy REQUIRED)
find_program(ARM_GCC_OBJDUMP arm-none-eabi-objdump REQUIRED)
find_program(ARM_GCC_SIZE arm-none-eabi-size REQUIRED)

set(CMAKE_C_COMPILER "${ARM_GCC_C}")
set(CMAKE_CXX_COMPILER "${ARM_GCC_CXX}")
set(CMAKE_ASM_COMPILER "${ARM_GCC_C}")
set(CMAKE_OBJCOPY "${ARM_GCC_OBJCOPY}")
set(CMAKE_OBJDUMP "${ARM_GCC_OBJDUMP}")
set(CMAKE_SIZE "${ARM_GCC_SIZE}")

# Verificar o compilador sem tentar vincular um programa de desktop.
# O firmware final sera vinculado com o startup e o linker script do projeto.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
