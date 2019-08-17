# CMakeList.txt : CMake project for gb_emu, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (gb_emu "gb_emu.cpp" "gb_emu.h" "cpu.h" "cpu.cpp" "memory.h" "memory.cpp" "cb_opcodes.h" "cb_opcodes.cpp")

# TODO: Add tests and install targets if needed.
