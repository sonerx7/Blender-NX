cmake_minimum_required(VERSION 3.13)
project(nexvyn-blender VERSION 4.2.0 LANGUAGES C CXX)

# Nexvyn Blender (Blender 4.2 LTS NX)
# Native 3D Creation Suite for Nexvyn OS N3-N4 High-Performance Execution
#   nxv-native build <dir>      -> build/nexvyn-blender.nxvmod
#   nxv-native package <dir>    -> build/nexvyn-blender.nxvapp

if(NOT DEFINED NEXVYN_KIT)
  get_filename_component(NEXVYN_KIT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()
set(NEXVYN_ABI ON CACHE BOOL "" FORCE)
include(${NEXVYN_KIT}/toolchain/nexvyn-native.cmake)

# Dear ImGui from the kit
set(IMGUI_DIR ${NEXVYN_KIT}/sysroot/vendor/imgui)
set(IMGUI_SRC ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_draw.cpp ${IMGUI_DIR}/imgui_tables.cpp ${IMGUI_DIR}/imgui_widgets.cpp
              src/imgui_gl_glue.cpp)
set_source_files_properties(${IMGUI_SRC} PROPERTIES COMPILE_OPTIONS "-w")

set(GLAD_LIB "$ENV{DEVKITPRO}/portlibs/switch/lib/libglad.a")
if(NOT EXISTS "${GLAD_LIB}")
  message(FATAL_ERROR "libglad.a not found: install the devkitPro switch-glad package")
endif()

set(BLENDER_SRC
    src/main.cpp
    src/Viewport3D.cpp
    ${IMGUI_SRC}
)

nexvyn_add_windowed(nexvyn-blender_win NAME nexvyn-blender SOURCES ${BLENDER_SRC} LINK ${GLAD_LIB})
target_include_directories(nexvyn-blender_win_objects PRIVATE
    src
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    "$ENV{DEVKITPRO}/portlibs/switch/include"
)
target_compile_definitions(nexvyn-blender_win_objects PRIVATE IMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS)
target_compile_options(nexvyn-blender_win_objects PRIVATE -O3 -Wall -Wextra -Wno-unused-parameter "$<$<COMPILE_LANGUAGE:CXX>:-fno-rtti;-fno-exceptions>")
