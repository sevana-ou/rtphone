project (media_lib)

# Rely on C++ 11
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

file(GLOB MEDIA_LIB_SOURCES "*.cpp" "*.h")

# SRTP requires this
add_definitions(-DHAVE_STDINT_H -DHAVE_UINT64_T)

if(CMAKE_SYSTEM MATCHES "Linux*")
    add_definitions(-DHAVE_NETINET_IN_H)
endif(CMAKE_SYSTEM MATCHES "Linux*")

if(CMAKE_SYSTEM MATCHES "Darwin*")
    # OS X Specific flags
    add_definitions(-DHAVE_NETINET_IN_H)
endif(CMAKE_SYSTEM MATCHES "Darwin*")

if (CMAKE_SYSTEM MATCHES "Win*")
    # Windows Specific flags - MSVC expected
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -DHAVE_WINSOCK2_H
        -D_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS -DUNICODE -D_UNICODE )
endif(CMAKE_SYSTEM MATCHES "Win*")

add_library(media_lib ${MEDIA_LIB_SOURCES})

target_include_directories(media_lib PRIVATE ../../libs/
    ../ ../../libs/srtp/include
    ../../libs/srtp/crypto/include
    ../../libs/webrtc)


