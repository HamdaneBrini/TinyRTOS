cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME firmware)

# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

option(TINYRTOS_UNIT_TEST "Build and run the unit-test firmware" OFF)

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Keep clangd synchronized with the most recently built CMake preset.
add_custom_target(update_compile_commands ALL
    COMMAND cmake -E copy_if_different
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/build/compile_commands.json
    COMMENT "Updating clangd compilation database"
)

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    Drivers/GPIO/Src/gpio.c
    Drivers/TIMER/Src/timer.c
    Drivers/UART/Src/uart.c
    Services/Src/Console/console.c
    Utils/Src/string_utils.c
    Utils/Src/io_utils.c
    Kernel/Src/heap_allocator.c
    Kernel/Src/mem_allocator.c
    kernel/Src/tiny_time.c
    Services/Src/logging.c
    Kernel/Src/scheduler.c
    Kernel/Src/mpu.c
    Kernel/Src/config.c
    Kernel/Src/syscall.c
    Kernel/Src/task.c

)

if(TINYRTOS_UNIT_TEST)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        Application/test/test_scheduler.c
        Services/Src/tiny_unity.c
    )
endif()

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/Drivers/GPIO/Inc
    ${CMAKE_SOURCE_DIR}/Drivers/TIMER/Inc
    ${CMAKE_SOURCE_DIR}/Drivers/UART/Inc
    ${CMAKE_SOURCE_DIR}/Services/Inc
    ${CMAKE_SOURCE_DIR}/Services/Inc/Console
    ${CMAKE_SOURCE_DIR}/Services/Inc/Logging
    ${CMAKE_SOURCE_DIR}/Utils/Inc
    ${CMAKE_SOURCE_DIR}/Kernel/Inc
    $<$<BOOL:${TINYRTOS_UNIT_TEST}>:${CMAKE_SOURCE_DIR}/Application/test>
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    $<$<BOOL:${TINYRTOS_UNIT_TEST}>:UNIT_TEST>
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
