project(rtphone)

# Rely on C++ 11
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set (rtphone_libs libs)
set (rtphone_engine engine)

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

target_link_libraries(rtphone ice_stack jrtplib g729_codec gsm_codec
    gsmhr_codec g722_codec srtp resiprocate
    opencore-amrnb opencore-amrwb
    helper_lib audio_lib webrtc speexdsp
    ssl crypto opus uuid dl)
