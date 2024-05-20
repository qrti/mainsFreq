add_executable(source)

# by default the header is generated into the build dir
pico_generate_pio_header(source ${CMAKE_CURRENT_LIST_DIR}/sample.pio)

# however, alternatively you can choose to generate it somewhere else (in this case in the source tree for check in)
#pico_generate_pio_header(pio_blink ${CMAKE_CURRENT_LIST_DIR}/blink.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})

target_sources(source PRIVATE main.cpp spi.cpp epaper.cpp draw.cpp text.cpp dcf.cpp datetimedcf.cpp sample.cpp view.cpp key.cpp)

# enable usb output, disable uart output for printf
pico_enable_stdio_usb(source 1)
pico_enable_stdio_uart(source 0)

target_link_libraries(source PRIVATE pico_stdlib pico_multicore hardware_spi hardware_pio hardware_dma hardware_adc)
pico_add_extra_outputs(source)

# add url via pico_set_program_url
example_auto_set_url(source)
