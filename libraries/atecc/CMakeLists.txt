cmake_minimum_required(VERSION 3.13)

# Initialize the Pico SDK
include(../../pico-sdk/pico_sdk_init.cmake)

# Set the project name
project(atecc LANGUAGES C)

# Initialize the Pico SDK
pico_sdk_init()

# Add the library
add_library(atecc STATIC
    src/hal_pico_i2c.c
    src/atecc_cmd.c
    src/atecc_crc.c
)

# Specify the include directories
target_include_directories(atecc PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${PICO_SDK_PATH}/include
)

# Link the Pico SDK libraries
target_link_libraries(atecc pico_stdlib hardware_i2c)