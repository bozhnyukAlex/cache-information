cmake_minimum_required(VERSION 3.22)
project(cache_information)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O2")

include_directories(.)

add_executable(cache_information
        cache_info.cpp)
