cmake_minimum_required(VERSION 3.16)

# Set toolchain and system configuration BEFORE project()
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv)

# Find and set the RISC-V toolchain
find_program(CMAKE_C_COMPILER riscv-none-elf-gcc REQUIRED)
find_program(CMAKE_CXX_COMPILER riscv-none-elf-g++ REQUIRED)
find_program(CMAKE_ASM_COMPILER riscv-none-elf-gcc REQUIRED)
find_program(CMAKE_OBJCOPY riscv-none-elf-objcopy REQUIRED)
find_program(CMAKE_OBJDUMP riscv-none-elf-objdump REQUIRED)
find_program(CMAKE_SIZE riscv-none-elf-size REQUIRED)

# Prevent CMake from testing the compiler and overriding settings
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)
set(CMAKE_ASM_COMPILER_FORCED TRUE)
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)
set(CMAKE_ASM_COMPILER_WORKS 1)

# Disable CMake's built-in compiler detection and platform modules
set(CMAKE_CROSSCOMPILING TRUE)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Project configuration
project(ch32v103-template C CXX ASM)

# Set C and C++ standards
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

# Target CPU and architecture flags
set(CPU_FLAGS "-march=rv32imac_zicsr -mabi=ilp32 -mcmodel=medlow")

# Common compiler flags
set(COMMON_FLAGS "${CPU_FLAGS} -ffunction-sections -fdata-sections -fno-common -fmessage-length=0")

# Completely override CMake's default flags
set(CMAKE_C_FLAGS "${COMMON_FLAGS} -std=gnu99" CACHE STRING "C flags" FORCE)
set(CMAKE_CXX_FLAGS "${COMMON_FLAGS} -std=gnu++17 -fno-exceptions -fno-rtti" CACHE STRING "CXX flags" FORCE)
set(CMAKE_ASM_FLAGS "${CPU_FLAGS}" CACHE STRING "ASM flags" FORCE)

# Override all build type flags to prevent CMake from adding unwanted flags
set(CMAKE_C_FLAGS_DEBUG "-Og -g3 -DDEBUG" CACHE STRING "C debug flags" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG "-Og -g3 -DDEBUG" CACHE STRING "CXX debug flags" FORCE)
set(CMAKE_ASM_FLAGS_DEBUG "-g3" CACHE STRING "ASM debug flags" FORCE)

set(CMAKE_C_FLAGS_RELEASE "-Os -DNDEBUG" CACHE STRING "C release flags" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-Os -DNDEBUG" CACHE STRING "CXX release flags" FORCE)
set(CMAKE_ASM_FLAGS_RELEASE "" CACHE STRING "ASM release flags" FORCE)

set(CMAKE_C_FLAGS_RELWITHDEBINFO "-Os -g -DNDEBUG" CACHE STRING "C relwithdebinfo flags" FORCE)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-Os -g -DNDEBUG" CACHE STRING "CXX relwithdebinfo flags" FORCE)
set(CMAKE_ASM_FLAGS_RELWITHDEBINFO "-g" CACHE STRING "ASM relwithdebinfo flags" FORCE)

set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG" CACHE STRING "C minsizerel flags" FORCE)
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG" CACHE STRING "CXX minsizerel flags" FORCE)
set(CMAKE_ASM_FLAGS_MINSIZEREL "" CACHE STRING "ASM minsizerel flags" FORCE)

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS "${CPU_FLAGS} -nostartfiles -Xlinker --gc-sections -Wl,-Map,${PROJECT_NAME}.map --specs=nano.specs --specs=nosys.specs" CACHE STRING "Linker flags" FORCE)

# Set linker script
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/system/Link.ld)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T${LINKER_SCRIPT}")

# Include directories
include_directories(
    core
    cpu
    driver/inc
    lib/debug
    system
    apps/framework
)

# Define preprocessor macros
add_definitions(-DCH32V10x)

# Collect source files
file(GLOB_RECURSE DRIVER_SOURCES "driver/src/*.c")
file(GLOB_RECURSE LIB_SOURCES "lib/debug/*.c")
file(GLOB_RECURSE CPU_SOURCES "cpu/*.c")

# Core sources
set(CORE_SOURCES
    core/ch32v10x_it.c
    core/main.cpp
    core/app.c
)

# System sources
set(SYSTEM_SOURCES
    system/system_ch32v10x.c
    system/startup_ch32v10x.S
    system/syscalls.c
)

# Application sources
set(APP_SOURCES
    apps/framework/app_framework.c
    apps/hello.c
    apps/adc_polling.c
    apps/adc_interrupt.c
    apps/adc_dma.c
    apps/gpio_polling.c
    apps/gpio_interrupt.c
    apps/i2c_polling.c
    apps/i2c_interrupt.c
    apps/i2c_dma.c
    apps/rtc.c
    apps/spi_polling.c
    apps/spi_interrupt.c
    apps/spi_dma.c
    apps/timer_interrupt.c
    apps/timer_pwm.c
    apps/uart_polling.c
    apps/uart_interrupt.c
    apps/uart_dma.c
    apps/flash.c
    apps/watchdog.c
)

# All sources
set(SOURCES
    ${CORE_SOURCES}
    ${SYSTEM_SOURCES}
    ${DRIVER_SOURCES}
    ${LIB_SOURCES}
    ${CPU_SOURCES}
    ${APP_SOURCES}
)

# Create executable
add_executable(${PROJECT_NAME}.elf ${SOURCES})

# Set linker script dependency
set_target_properties(${PROJECT_NAME}.elf PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})

# Create hex file
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.hex
    COMMENT "Creating HEX file"
)

# Create binary file
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
    COMMENT "Creating BIN file"
)

# Print size information
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_SIZE} --format=berkeley $<TARGET_FILE:${PROJECT_NAME}.elf>
    COMMENT "Size information:"
)

# Create disassembly file
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJDUMP} -h -S $<TARGET_FILE:${PROJECT_NAME}.elf> > ${PROJECT_NAME}.list
    COMMENT "Creating disassembly file"
)

# Print build information
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "Linker script: ${LINKER_SCRIPT}")
message(STATUS "CPU flags: ${CPU_FLAGS}")