project (g722_codec)

# Rely on C++ 11
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set (G722_SOURCES
    g722_bitstream.c
    g722_decode.c
    g722_encode.c
)

add_library(g722_codec ${G722_SOURCES})
set_property(TARGET g722_codec PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

