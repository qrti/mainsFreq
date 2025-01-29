# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(mainsFreq C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
add_executable(mainsFreq main.cpp spi.cpp epaper.cpp draw.cpp text.cpp dcf.cpp datetimedcf.cpp sample.cpp view.cpp key.cpp)

pico_set_program_name(mainsFreq "mainsFreq")
pico_set_program_version(mainsFreq "1.53")

# Generate PIO header
pico_generate_pio_header(mainsFreq ${CMAKE_CURRENT_LIST_DIR}/sample.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(mainsFreq 0)
pico_enable_stdio_usb(mainsFreq 0)

# Add the standard library to the build
target_link_libraries(mainsFreq pico_stdlib)

# Add the standard include files to the build
target_include_directories(mainsFreq PRIVATE 
  ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(mainsFreq pico_multicore hardware_spi hardware_pio hardware_dma hardware_adc)

pico_add_extra_outputs(mainsFreq)
