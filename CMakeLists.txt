cmake_minimum_required(VERSION 3.22)
project(zagros)

set(CMAKE_CXX_FLAGS "-fno-exceptions")
set(CMAKE_CXX_FLAGS "-fno-rtti")
set(CMAKE_CXX_STANDARD 11)

add_subdirectory(src)
add_subdirectory(test)

add_library(zagros src/vm.cpp src/io.h)
