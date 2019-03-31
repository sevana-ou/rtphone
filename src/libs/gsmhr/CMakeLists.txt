project (gsmhr_codec)

# Rely on C++ 11
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set (GSMHR_SOURCES
    gsmhr.cpp
    gsmhr_sp_rom.c
)

add_library(gsmhr_codec ${GSMHR_SOURCES})
