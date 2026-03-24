# ESP32-S3 + DW3220 UWB Accessory Firmware
# For use with iOS NearbyInteraction and Android UWB Jetpack

cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    $ENV{IDF_PATH}/examples/common_components/led_strip
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(esp32_dw3220_uwb)
