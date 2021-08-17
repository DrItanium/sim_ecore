cmake_minimum_required(VERSION 3.20)
project(sim3)

set(CMAKE_CXX_STANDARD 17)

add_executable(sim3 ConstexprSimulator.cc
                    OpcodesRaw.h)
