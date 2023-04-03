cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)

project(garagestate)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(garagestatus
    main.cpp
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(garagestatus pico_stdlib pico_runtime hardware_adc hardware_sleep hardware_rtc)

# create map/bin/hex/uf2 file in addition to ELF.
pico_enable_stdio_usb(garagestatus 1)
pico_enable_stdio_uart(garagestatus 0)
pico_add_extra_outputs(garagestatus)