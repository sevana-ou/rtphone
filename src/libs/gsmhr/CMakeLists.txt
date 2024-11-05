project (gsmhr_codec)

# Rely on C++ 11
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set (GSMHR_SOURCES
    gsmhr.cpp
    gsmhr_sp_rom.c
)

add_library(gsmhr_codec ${GSMHR_SOURCES})
set_property(TARGET gsmhr_codec PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
