cmake_minimum_required(VERSION 3.16)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

set(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
set(EXTRA_COMPONENT_DIRS
    "${PROJECT_ROOT}/components/led_strip"
    "${PROJECT_ROOT}/components/helix_mp3"
    "${PROJECT_ROOT}/drivers/audio/korvo1")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(naphome-korvo1)
