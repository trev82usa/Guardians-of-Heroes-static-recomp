# Guardian Heroes - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(guardianheroes LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
set(GUARDIANHEROES_SOURCES
    src/main.cpp
    src/gh_menu.cpp
    src/crash_handler.cpp
    src/game_exit.cpp
    src/keyboard_input.cpp
    src/perf_tools.cpp
    src/net_fixes.cpp
    src/usbcam_stubs.cpp
)

if(WIN32)
    add_executable(guardianheroes WIN32 ${GUARDIANHEROES_SOURCES})
else()
    add_executable(guardianheroes ${GUARDIANHEROES_SOURCES})
endif()

target_compile_definitions(guardianheroes PRIVATE
    GH_XEX_VERSION=2
    GH_PRESENT_FN=sub_825DE710
    GH_LAUNCH_FN=sub_825CF5C8)

# Stage the Xenos GPU emulation plugin next to the program (loaded at runtime).
rexglue_setup_target(guardianheroes GPU_PLUGINS xenos)
