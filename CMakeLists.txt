cmake_minimum_required(VERSION 3.13...3.27)

# Include Pico SDK (Must be done before calling project())
include(pico-sdk/pico_sdk_init.cmake)

# Set project name and version
project(pico_atecc VERSION 1.0)

# Initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# Create the executable
add_executable(pico_atecc
    src/pico_atecc.c
)

# Include the ATECC library
add_subdirectory(libraries/atecc)

# Link necessary libraries
target_link_libraries(pico_atecc pico_stdlib hardware_i2c atecc)

# Include the necessary directories
target_include_directories(pico_atecc PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/libraries/atecc/src
)


# Enable UART output, disable USB output
pico_enable_stdio_usb(pico_atecc 0)
pico_enable_stdio_uart(pico_atecc 1)

# Generate additional output formats (UF2, bin, hex, map)
pico_add_extra_outputs(pico_atecc)
