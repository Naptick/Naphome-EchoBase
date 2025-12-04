cmake_minimum_required(VERSION 3.16)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

set(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}")

# Board-specific component directories
set(EXTRA_COMPONENT_DIRS
    "${PROJECT_ROOT}/components/led_strip"
    "${PROJECT_ROOT}/components/helix_mp3")

# Add board-specific audio driver
if(CONFIG_BOARD_M5_ECHO_BASE)
    list(APPEND EXTRA_COMPONENT_DIRS "${PROJECT_ROOT}/drivers/audio/m5_echo_base")
elseif(CONFIG_BOARD_KORVO1)
    list(APPEND EXTRA_COMPONENT_DIRS "${PROJECT_ROOT}/drivers/audio/korvo1")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(naphome-korvo1)
