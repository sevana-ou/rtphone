cmake_minimum_required (VERSION 3.15)
project (helper_lib)

# Rely on C++ 11
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set (USE_NULL_UUID OFF CACHE BOOL "When enabled linking to libuuid is avoided")

set (CMAKE_POSITION_INDEPENDENT_CODE ON)

file (GLOB HELPER_LIB_SOURCES "*.cpp" "*.h")

add_library(helper_lib ${HELPER_LIB_SOURCES})
set_property(TARGET helper_lib PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Private include directories
target_include_directories(helper_lib PUBLIC ../../libs/ ../../engine ../ .)
target_compile_definitions(helper_lib PRIVATE -D_CRT_SECURE_NO_WARNINGS -D_UNICODE)
