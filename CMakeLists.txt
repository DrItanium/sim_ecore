cmake_minimum_required(VERSION 3.15)
project(sim3)

set(CMAKE_CXX_STANDARD 17)

if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra)
endif()
add_library(sim3core
        OpcodesRaw.h Core.h Core.cc Types.h Instruction.h Register.h Register.cc SimplifiedSxCore.cc SimplifiedSxCore.h sim3sx.cc SBCore.h SBCore.cc)
add_executable(sim3_zct
        main.cc)
add_executable(sim3_sx
                sim3sx.cc)

target_link_libraries(sim3_zct sim3core)
target_link_libraries(sim3_sx sim3core)