project (media_lib)

# Rely on C++ 11
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

# Produce PIC code always
set (CMAKE_POSITION_INDEPENDENT_CODE ON)

#set (USE_RESIP_INTEGRATION CACHE BOOL OFF "Integrate with resip structures")
file(GLOB MEDIA_LIB_SOURCES "*.cpp" "*.h")

if(CMAKE_SYSTEM MATCHES "Linux*")
    add_definitions(-DHAVE_NETINET_IN_H)
endif()

if(CMAKE_SYSTEM MATCHES "Darwin*")
    # OS X Specific flags
    add_definitions(-DHAVE_NETINET_IN_H)
endif()

if (CMAKE_SYSTEM MATCHES "Windows*")
    # Windows Specific flags - MSVC expected
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -DHAVE_WINSOCK2_H
        -D_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS -DUNICODE -D_UNICODE )
endif()

add_library(media_lib ${MEDIA_LIB_SOURCES})

target_include_directories(media_lib
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/
            ${CMAKE_CURRENT_SOURCE_DIR}/../
            ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/srtp/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/srtp/crypto/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/webrtc
            ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/opus/include/
            ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/resiprocate/
            )

if (USE_RESIP_INTEGRATION)
    message("USE_RESIP_INTEGRATION is turned on!")
    target_compile_definitions(media_lib PUBLIC -DUSE_RESIP_INTEGRATION)
endif()

