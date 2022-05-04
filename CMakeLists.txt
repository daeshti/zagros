cmake_minimum_required(VERSION 3.22)
project(zagros)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-fno-exceptions")

add_subdirectory(src)
add_subdirectory(test)

add_library(zagros src/zagros.cpp)
