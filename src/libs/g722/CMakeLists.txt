project (g722_codec)

# Rely on C++ 11
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set (G722_SOURCES
    g722_bitstream.c
    g722_decode.c
    g722_encode.c
)

add_library(g722_codec ${G722_SOURCES})
