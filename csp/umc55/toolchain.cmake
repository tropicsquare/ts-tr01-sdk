# 'Generic' system name for embedded or baremetal targets
# imc extensions for the Ibex core
set( CMAKE_SYSTEM_NAME          Generic )
set( CMAKE_SYSTEM_PROCESSOR     rv32imc )
set( CMAKE_EXECUTABLE_SUFFIX    ".elf" )

# For cmake to bypass test compilation check
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

set(RISC_V_TOOLCHAIN_PATH      $ENV{RISCV_TOOLCHAIN_ROOT}/bin)

set(CMAKE_AR                    "${RISC_V_TOOLCHAIN_PATH}/riscv32-unknown-elf-ar"  )
set(CMAKE_ASM_COMPILER          "${RISC_V_TOOLCHAIN_PATH}/riscv32-unknown-elf-gcc" )
set(CMAKE_C_COMPILER            "${RISC_V_TOOLCHAIN_PATH}/riscv32-unknown-elf-gcc" )
set(CMAKE_CXX_COMPILER          "${RISC_V_TOOLCHAIN_PATH}/riscv32-unknown-elf-g++" ) 
set(CMAKE_LINKER                "${RISC_V_TOOLCHAIN_PATH}/riscv32-unknown-elf-ld"  )

# set objcopy settings into cache so the build knows what the objcopy filepath is
set(CMAKE_OBJCOPY               "${RISC_V_TOOLCHAIN_PATH}/riscv32-unknown-elf-objcopy" 
     CACHE FILEPATH "The toolchain objcopy command " FORCE )
set(CMAKE_OBJDUMP               "${RISC_V_TOOLCHAIN_PATH}/riscv32-unknown-elf-objdump"
     CACHE FILEPATH "The toolchain objdump command " FORCE )

set(CMAKE_RANLIB                "${RISC_V_TOOLCHAIN_PATH}/riscv32-unknown-elf-ranlib" )
set(CMAKE_SIZE                  "${RISC_V_TOOLCHAIN_PATH}/riscv32-unknown-elf-size"   )
set(CMAKE_STRIP                 "${RISC_V_TOOLCHAIN_PATH}/riscv32-unknown-elf-strip"  )

set ( CMAKE_C_FLAGS "-march=${CMAKE_SYSTEM_PROCESSOR} -mabi=ilp32 -fno-builtin -nostartfiles" )

set ( CMAKE_C_FLAGS_RELEASE_INIT "" )
set ( CMAKE_C_FLAGS_DEBUG_INIT "" )

set ( CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS}" )

# Linker options
SET( CMAKE_SHARED_LINKER_FLAGS  " -ffreestanding -nostdlib" )

set( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER )

# search headers and libraries in the target environment
set( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
set( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )
set( CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY )

