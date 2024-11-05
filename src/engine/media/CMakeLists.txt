project (media_lib)

if (NOT DEFINED LIB_PLATFORM)
    message(FATAL_ERROR media_lib project requires LIB_PLATFORM to be set - it uses libraries from that directory)
endif()

# Rely on C++ 20
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

# Produce PIC code always
set (CMAKE_POSITION_INDEPENDENT_CODE ON)
option (USE_RESIP_INTEGRATION "Integrate with resiprocate project" OFF)

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
    MT_AmrCodec.cpp
    MT_EvsCodec.cpp

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
    MT_AmrCodec.h
    MT_EvsCodec.h
    )

add_library(media_lib ${SOURCES})
set (LIBS_CODEC)

if (USE_AMR_CODEC)
    include (${LIB_PLATFORM}/platform_libs.cmake)
    message("Media: AMR NB and WB codecs will be included.")
    target_compile_definitions(media_lib PUBLIC USE_AMR_CODEC)
    list (APPEND LIBS_CODEC ${OPENCORE_AMRNB} ${OPENCORE_AMRWB})
endif()

if (USE_EVS_CODEC)
    message("Media: EVS codec will be included.")
    target_compile_definitions (media_lib PUBLIC USE_EVS_CODEC)
    list (APPEND LIBS_CODEC evs_codec)
endif()

if (USE_OPUS_CODEC)
    message("Media: Opus codec will be included.")
    target_compile_definitions(media_lib PUBLIC USE_OPUS_CODEC)
    list (APPEND LIBS_CODEC opus)
    add_subdirectory(../../libs/opus build_opus)
endif()

if(CMAKE_SYSTEM MATCHES "Linux*" OR CMAKE_SYSTEM MATCHES "Darwin*")
    target_compile_definitions(media_lib PUBLIC HAVE_NETINET_IN_H)
endif()


if (CMAKE_SYSTEM MATCHES "Windows*")
    # Windows Specific flags - MSVC expected
    target_compile_definitions(media_lib PRIVATE
        _CRT_SECURE_NO_WARNINGS
        HAVE_WINSOCK2_H
        _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS
        UNICODE
        _UNICODE )
endif()



# Dependency on ice_stack - Linux build requires it
# Codec libraries as well
target_link_libraries(media_lib ice_stack ${LIBS_CODEC})

set_property(TARGET media_lib PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

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

