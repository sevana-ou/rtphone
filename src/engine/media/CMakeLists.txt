project (media_lib)

# Rely on C++ 11
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

# Produce PIC code always
set (CMAKE_POSITION_INDEPENDENT_CODE ON)

# Set of variables to control references to codecs
set (USE_AMR_CODEC       OFF CACHE BOOL "Use AMR codec. Requires libraries.")
set (USE_EVS_CODEC       OFF CACHE BOOL "Use EVS codec."                    )
set (USE_OPUS_CODEC      OFF CACHE BOOL "Use Opus codec."                   )
set (USE_SEVANA_LIB      OFF CACHE BOOL "Build with Sevana libraries"       )

set (SOURCES
    MT_Statistics.cpp
    MT_WebRtc.cpp
    MT_Stream.cpp
    MT_SrtpHelper.cpp
    MT_SingleAudioStream.cpp
    MT_NativeRtpSender.cpp
    MT_Dtmf.cpp
    MT_CodecList.cpp
    MT_Codec.cpp
    MT_Box.cpp
    MT_AudioStream.cpp
    MT_AudioReceiver.cpp
    MT_AudioCodec.cpp
    MT_CngHelper.cpp

    MT_Statistics.h
    MT_WebRtc.h
    MT_Stream.h
    MT_SrtpHelper.h
    MT_SingleAudioStream.h
    MT_NativeRtpSender.h
    MT_Dtmf.h
    MT_CodecList.h
    MT_Codec.h
    MT_Box.h
    MT_AudioStream.h
    MT_AudioReceiver.h
    MT_AudioCodec.h
    MT_CngHelper.h
    )

if (USE_AMR_CODEC)
    message("AMR NB and WB codecs will be included.")
    add_definitions(-DUSE_AMR_CODEC)
    set(SOURCES ${SOURCES} MT_AmrCodec.cpp MT_AmrCodec.h)
endif()

if (USE_EVS_CODEC)
    message("EVS codec will be included.")
    add_definitions (-DUSE_EVS_CODEC)
    set (SOURCES ${SOURCES} MT_EvsCodec.cpp MT_EvsCodec.h)
endif()

if (USE_OPUS_CODEC)
    message("Opus codec will be included.")
    add_definitions(-DUSE_OPUS_CODEC)
endif()


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

add_library(media_lib ${SOURCES})

target_include_directories(media_lib
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/
            ${CMAKE_CURRENT_SOURCE_DIR}/../
            ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/srtp/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/srtp/crypto/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/webrtc
            ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/opus/include/
            ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/resiprocate/
            ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/libevs/
            )

target_include_directories(media_lib
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/libevs/lib_com
            ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/libevs/lib_dec
            ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/libevs/lib_enc
            ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/libevs/basic_op
            ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/libevs/basic_math
)
if (USE_RESIP_INTEGRATION)
    message("USE_RESIP_INTEGRATION is turned on!")
    target_compile_definitions(media_lib PUBLIC -DUSE_RESIP_INTEGRATION)
endif()

