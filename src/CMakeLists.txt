project(rtphone)

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


# Rely on C++ 11
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set (rtphone_libs libs)
set (rtphone_engine engine)

set (USE_AMR_CODEC OFF CACHE BOOL "Use AMR codec. Requires libraries.")
set (OPENSSL_SSL ssl CACHE STRING "Pointer to ssl library")
set (OPENSSL_CRYPTO crypto CACHE STRING "Pointer to crypto library")

message ("Using ssl library at ${OPENSSL_SSL}")
message ("Using crypto library at ${OPENSSL_CRYPTO}")

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
    ${rtphone_engine}/media/MT_SevanaMos.cpp
    ${rtphone_engine}/media/MT_AmrCodec.cpp
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
)

add_library(rtphone STATIC ${RTPHONE_SOURCES})

add_subdirectory(${rtphone_engine}/helper)
add_subdirectory(${rtphone_engine}/audio)
add_subdirectory(${rtphone_libs}/resiprocate)
add_subdirectory(${rtphone_libs}/ice)
add_subdirectory(${rtphone_libs}/jrtplib/src)
add_subdirectory(${rtphone_libs}/libg729)
add_subdirectory(${rtphone_libs}/libgsm)
add_subdirectory(${rtphone_libs}/gsmhr)
add_subdirectory(${rtphone_libs}/g722)
add_subdirectory(${rtphone_libs}/speexdsp)
add_subdirectory(${rtphone_libs}/srtp)
add_subdirectory(${rtphone_libs}/webrtc)

set(LIBS ice_stack jrtplib g729_codec gsm_codec
    gsmhr_codec g722_codec srtp resiprocate helper_lib audio_lib webrtc speexdsp
    uuid dl)

if (CMAKE_SYSTEM MATCHES "Win*")
    set (LIBS ${LIBS} ssleay32 libeay32 opus silk_float celt)
else (CMAKE_SYSTEM MATCHES "Win*")
    set (LIBS ${LIBS} dl opus uuid)
endif (CMAKE_SYSTEM MATCHES "Win*")

if (USE_AMR_CODEC)
    set(LIBS ${LIBS} opencore-amrnb opencore-amrwb)
endif (USE_AMR_CODEC)

target_link_libraries(rtphone
    ice_stack jrtplib g729_codec gsm_codec
    gsmhr_codec g722_codec srtp resiprocate
    helper_lib audio_lib webrtc speexdsp
    opus uuid ${OPENSSL_SSL} ${OPENSSL_CRYPTO} ${LIBS})


target_include_directories(rtphone
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine>
    PRIVATE ../../libs/speex/include ../../libs ../)

configure_msvc_runtime()
