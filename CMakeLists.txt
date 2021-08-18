cmake_minimum_required(VERSION 3.20)
project(sim3)

set(CMAKE_CXX_STANDARD 17)

if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra)
endif()
add_executable(sim3 Simulator.cc
                    OpcodesRaw.h main.cc Core.h Core.cc Types.h Instruction.h Register.h)