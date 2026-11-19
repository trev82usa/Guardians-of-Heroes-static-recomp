# Guardian Heroes - build for game version 0.0.0.2 (the original 2011 release).
# Same source code as the main build (../../src); only the recompiled game code (generated/),
# the analysis fixes (gh_*.toml) and the addresses below differ. Produces guardianheroes-v2.

cmake_minimum_required(VERSION 3.25)
project(guardianheroes LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

set(GH_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../../src)
set(GUARDIANHEROES_SOURCES
    ${GH_SRC}/main.cpp
    ${GH_SRC}/gh_menu.cpp
    ${GH_SRC}/crash_handler.cpp
    ${GH_SRC}/game_exit.cpp
    ${GH_SRC}/keyboard_input.cpp
    ${GH_SRC}/perf_tools.cpp
    ${GH_SRC}/net_fixes.cpp
    ${GH_SRC}/usbcam_stubs.cpp
)

if(WIN32)
    add_executable(guardianheroes WIN32 ${GUARDIANHEROES_SOURCES})
else()
    add_executable(guardianheroes ${GUARDIANHEROES_SOURCES})
endif()
set_target_properties(guardianheroes PROPERTIES OUTPUT_NAME guardianheroes-v2)
target_include_directories(guardianheroes PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${GH_SRC})
target_compile_definitions(guardianheroes PRIVATE
    GH_XEX_VERSION=2
    GH_PRESENT_FN=sub_825DE710
    GH_LAUNCH_FN=sub_825CF5C8)

rexglue_setup_target(guardianheroes GPU_PLUGINS xenos)
