# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("gb_emu")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

if (WIN32)
		set(SDL2_DIR "./lib/SDL2-2.0.10")
endif()

add_definitions(-DFS_BASE_PATH="${CMAKE_SOURCE_DIR}/resources")
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

set (CMAKE_CXX_STANDARD 17)

set(OpenGL_GL_PREFERENCE GLVND)

find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)

include_directories("./inc" "./lib" "./src" ${SDL2_INCLUDE_DIRS})

add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLAD)

# Include sub-projects.
# Add source to this project's executable.
add_executable (gb_emu
WIN32
"./src/gb_emu.cpp"
"./src/gb_emu.h"
"./src/gameboy.cpp"
"./src/gameboy.h"
"./src/opcodes.cpp"
"./src/cpu.h"
"./src/cpu.cpp"
"./src/ppu.h"
"./src/ppu.cpp"
"./src/memory.cpp"
"./src/cb_opcodes.cpp"
"./src/rom.h"
"./src/rom.cpp"
"./src/simple_texture.h"
"./src/profiler.h"
"./src/profiler.cpp"
"./src/gui/window.h"
"./src/gui/textured_rectangle.h"
"./src/gui/textured_rectangle.cpp"

"./src/sound/blargg_common.h"
"./src/sound/blargg_source.h"
"./src/sound/Blip_Buffer.cpp"
"./src/sound/Blip_Buffer.h"
"./src/sound/Blip_Synth.h"
"./src/sound/Gb_Apu.cpp"
"./src/sound/Gb_Apu.h"
"./src/sound/Gb_Oscs.cpp"
"./src/sound/Gb_Oscs.h"
"./src/sound/Multi_Buffer.cpp"
"./src/sound/Multi_Buffer.h"
"./src/sound/Sound_Queue.cpp"
"./src/sound/Sound_Queue.h"
"./src/sound/Wave_Writer.cpp"
"./src/sound/Wave_Writer.h"

"src/gui/glad.c"
# imgui
"./src/imgui/imgui.cpp"
"./src/imgui/imgui_widgets.cpp"
"./src/imgui/imgui_demo.cpp"
"./src/imgui/imgui_draw.cpp"
"./src/imgui/imgui_impl_sdl.cpp"
"./src/imgui/imgui_impl_opengl3.cpp"
)


target_link_libraries(gb_emu ${SDL2_LIBRARIES})
