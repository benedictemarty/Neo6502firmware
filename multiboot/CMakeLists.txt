cmake_minimum_required(VERSION 3.13)
# Neo6502 multi-boot selector (F-80). Build :
#   cmake -S multiboot -B multiboot/build -DPICO_SDK_PATH=$PWD/firmware/build/_deps/pico_sdk-src && make -C multiboot/build
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(neoboot C CXX ASM)
set(CMAKE_C_STANDARD 11)
pico_sdk_init()
add_executable(neoboot neoboot.c)
target_compile_definitions(neoboot PRIVATE PICO_FLASH_SPI_CLKDIV=4 PICO_FLASH_SIZE_BYTES=2097152)
target_link_libraries(neoboot pico_stdlib hardware_sync)
pico_set_linker_script(neoboot ${CMAKE_CURRENT_LIST_DIR}/memmap_selector.ld)
pico_add_extra_outputs(neoboot)
