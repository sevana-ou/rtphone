cmake_minimum_required(VERSION 3.20)
project(rtp_decode)

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(../../src build_rtphone)

add_executable(rtp_decode main.cpp)
target_link_libraries(rtp_decode PRIVATE rtphone)
