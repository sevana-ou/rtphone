project(rtphone)

cmake_minimum_required(VERSION 3.0)
 
macro(configure_msvc_runtime)
    if(MSVC)
        # Default to statically-linked runtime.
        if("${MSVC_RUNTIME}" STREQUAL "")
            set(MSVC_RUNTIME "static")
        endif()
        # Set compiler options.
        set(variables
            CMAKE_C_FLAGS_DEBUG
            CMAKE_C_FLAGS_MINSIZEREL
            CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_RELWITHDEBINFO
            CMAKE_CXX_FLAGS_DEBUG
            CMAKE_CXX_FLAGS_MINSIZEREL
            CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_RELWITHDEBINFO
            )
        if(${MSVC_RUNTIME} STREQUAL "static")
            message(STATUS
                "rtphone: MSVC -> forcing use of statically-linked runtime."
                )
            foreach(variable ${variables})
                if(${variable} MATCHES "/MD")
                    string(REGEX REPLACE "/MD" "/MT" ${variable} "${${variable}}")
                endif()
            endforeach()
        else()
            message(STATUS
                "rtphone: MSVC -> forcing use of dynamically-linked runtime."
                )
            foreach(variable ${variables})
                if(${variable} MATCHES "/MT")
                    string(REGEX REPLACE "/MT" "/MD" ${variable} "${${variable}}")
                endif()
            endforeach()
        endif()

        foreach(variable ${variables})
            string(REGEX REPLACE "/Z[iI7]" ""
                ${variable}
                "${${variable}}")

            set(${variable} "${${variable}} /Zi /Oy-")
        endforeach()
    endif()
endmacro()


# Rely on C++ 17
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set (rtphone_libs libs)
set (rtphone_engine engine)

option (USE_AMR_CODEC   "Use AMR codec. Requires libraries."    ON)
option (USE_EVS_CODEC   "Use EVS codec."                        ON)
option (USE_OPUS_CODEC  "Use Opus codec."                       ON)
option (USE_MUSL        "Build with MUSL library"               OFF)

# PIC code by default
set (CMAKE_POSITION_INDEPENDENT_CODE ON)

if (NOT DEFINED LIB_PLATFORM)
    set (LIB_PLATFORM ${CMAKE_CURRENT_SOURCE_DIR}/../../libraries)
endif()
include (${LIB_PLATFORM}/platform_libs.cmake)

message("Libraries: ${LIB_PLATFORM}")
set (OPENSSL_INCLUDE ${LIB_PLATFORM}/openssl/1.1/include)
message ("Using OpenSSL include files from ${OPENSSL_INCLUDE}")
message ("Using OpenSSL libs: ${OPENSSL_SSL} and ${OPENSSL_CRYPTO}")
add_definitions(-DUSE_OPENSSL)
include_directories(${OPENSSL_INCLUDE})

if (CMAKE_SYSTEM MATCHES "Windows*")
    add_definitions (-DTARGET_WIN -D_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS -D_UNICODE -D_CRT_SECURE_NO_WARNINGS)
endif()

if (CMAKE_SYSTEM MATCHES "Linux*")
    add_definitions (-DTARGET_LINUX -DHAVE_NETINET_IN_H)
endif()


if (CMAKE_SYSTEM MATCHES "Darwin*")
    add_definitions (-DTARGET_OSX)
endif()

if (CMAKE_SYSTEM MATCHES "Android")
    message("Adding the Oboe library")
    set (OBOE_DIR libs/oboe)
    add_subdirectory (${OBOE_DIR} ./oboe)
    include_directories (${OBOE_DIR}/include)
    add_definitions(-DTARGET_ANDROID -DHAVE_NETINET_IN_H)
endif()

if (USE_MUSL)
    add_definitions(-DTARGET_MUSL)
endif()

set (RTPHONE_SOURCES
    ${rtphone_engine}/media/MT_Statistics.cpp
    ${rtphone_engine}/media/MT_WebRtc.cpp
    ${rtphone_engine}/media/MT_Stream.cpp
    ${rtphone_engine}/media/MT_SrtpHelper.cpp
    ${rtphone_engine}/media/MT_SingleAudioStream.cpp
    ${rtphone_engine}/media/MT_NativeRtpSender.cpp
    ${rtphone_engine}/media/MT_Dtmf.cpp
    ${rtphone_engine}/media/MT_CodecList.cpp
    ${rtphone_engine}/media/MT_Codec.cpp
    ${rtphone_engine}/media/MT_Box.cpp
    ${rtphone_engine}/media/MT_AudioStream.cpp
    ${rtphone_engine}/media/MT_AudioReceiver.cpp
    ${rtphone_engine}/media/MT_AudioCodec.cpp
    ${rtphone_engine}/media/MT_CngHelper.cpp
    ${rtphone_engine}/agent/Agent_Impl.cpp
    ${rtphone_engine}/agent/Agent_AudioManager.cpp
    ${rtphone_engine}/endpoint/EP_Account.cpp
    ${rtphone_engine}/endpoint/EP_AudioProvider.cpp
    ${rtphone_engine}/endpoint/EP_DataProvider.cpp
    ${rtphone_engine}/endpoint/EP_Engine.cpp
    ${rtphone_engine}/endpoint/EP_NetworkQueue.cpp
    ${rtphone_engine}/endpoint/EP_Observer.cpp
    ${rtphone_engine}/endpoint/EP_Session.cpp

    ${rtphone_engine}/media/MT_Statistics.h
    ${rtphone_engine}/media/MT_WebRtc.h
    ${rtphone_engine}/media/MT_Stream.h
    ${rtphone_engine}/media/MT_SrtpHelper.h
    ${rtphone_engine}/media/MT_SingleAudioStream.h
    ${rtphone_engine}/media/MT_NativeRtpSender.h
    ${rtphone_engine}/media/MT_Dtmf.h
    ${rtphone_engine}/media/MT_CodecList.h
    ${rtphone_engine}/media/MT_Codec.h
    ${rtphone_engine}/media/MT_Box.h
    ${rtphone_engine}/media/MT_AudioStream.h
    ${rtphone_engine}/media/MT_AudioReceiver.h
    ${rtphone_engine}/media/MT_AudioCodec.h

    ${rtphone_engine}/media/MT_CngHelper.h
    ${rtphone_engine}/agent/Agent_Impl.h
    ${rtphone_engine}/agent/Agent_AudioManager.h
    ${rtphone_engine}/endpoint/EP_Account.h
    ${rtphone_engine}/endpoint/EP_AudioProvider.h
    ${rtphone_engine}/endpoint/EP_DataProvider.h
    ${rtphone_engine}/endpoint/EP_Engine.h
    ${rtphone_engine}/endpoint/EP_NetworkQueue.h
    ${rtphone_engine}/endpoint/EP_Observer.h
    ${rtphone_engine}/endpoint/EP_Session.h
    )

if (USE_AMR_CODEC)
    add_definitions(-DUSE_AMR_CODEC)
    set (RTPHONE_SOURCES ${RTPHONE_SOURCES} ${rtphone_engine}/media/MT_AmrCodec.cpp ${rtphone_engine}/media/MT_AmrCodec.h)
endif()

if (USE_EVS_CODEC)
    add_definitions(-DUSE_EVS_CODEC)
    set (RTPHONE_SOURCES ${RTPHONE_SOURCES} ${rtphone_engine}/media/MT_EvsCodec.cpp ${rtphone_engine}/media/MT_EvsCodec.h)
endif()

if (USE_OPUS_CODEC)
    add_definitions(-DUSE_OPUS_CODEC)
endif()

add_library (rtphone STATIC ${RTPHONE_SOURCES})

add_subdirectory(${rtphone_libs}/resiprocate)
add_subdirectory(${rtphone_libs}/ice)
add_subdirectory(${rtphone_libs}/jrtplib/src)
add_subdirectory(${rtphone_libs}/libg729)

if (USE_EVS_CODEC)
    add_subdirectory(${rtphone_libs}/libevs)
endif()

add_subdirectory(${rtphone_libs}/libgsm)
add_subdirectory(${rtphone_libs}/gsmhr)
add_subdirectory(${rtphone_libs}/g722)
add_subdirectory(${rtphone_libs}/speexdsp)
add_subdirectory(${rtphone_libs}/srtp)
add_subdirectory(${rtphone_libs}/webrtc)
add_subdirectory(${rtphone_engine}/helper)
add_subdirectory(${rtphone_engine}/audio)
add_subdirectory(${rtphone_engine}/media)

set (LIBS ice_stack jrtplib g729_codec gsm_codec
    gsmhr_codec g722_codec srtp resiprocate helper_lib audio_lib webrtc speexdsp)

if (CMAKE_SYSTEM MATCHES "Win*")
    set (LIBS ${LIBS} )
else ()
    set (LIBS ${LIBS} dl)
endif ()

if (CMAKE_SYSTEM MATCHES "Android")
    set (LIBS ${LIBS} oboe)
endif()


target_link_libraries(rtphone
    ${LIBS}
    ${OPENSSL_SSL}
    ${OPENSSL_CRYPTO}
     )


target_include_directories(rtphone
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine>
        ${CMAKE_CURRENT_SOURCE_DIR}/libs
        ${LIB_PLATFORM}/opus/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/libevs/lib_com
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/libevs/lib_enc
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/libevs/lib_dec
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/speex/include
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/opus/include/
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/json
    )

# For MSVC static builds
configure_msvc_runtime()
