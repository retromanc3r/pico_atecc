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
    src/atecc_crc.c
)

# Link necessary libraries
target_link_libraries(pico_atecc 
    pico_stdlib 
    hardware_i2c
)

# Generate additional output formats (UF2, bin, hex, map)
pico_add_extra_outputs(pico_atecc)
