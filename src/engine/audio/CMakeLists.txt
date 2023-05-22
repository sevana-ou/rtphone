project (audio_lib)

# Rely on C++ 11
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set (AUDIOLIB_SOURCES
    Audio_Resampler.cpp
    Audio_Resampler.h
    Audio_Quality.cpp
    Audio_Quality.h
    Audio_Mixer.cpp
    Audio_Mixer.h
    Audio_Interface.cpp
    Audio_Interface.h
    Audio_Helper.cpp
    Audio_Helper.h
    Audio_DataWindow.cpp
    Audio_DataWindow.h
    Audio_DevicePair.cpp
    Audio_DevicePair.h
    Audio_Player.cpp
    Audio_Player.h
    Audio_Null.cpp
    Audio_Null.h
    Audio_CoreAudio.cpp
    Audio_CoreAudio.h
    Audio_DirectSound.cpp
    Audio_DirectSound.h
    Audio_AndroidOboe.cpp
    Audio_AndroidOboe.h
    Audio_WavFile.cpp
    Audio_WavFile.h
)

add_library(audio_lib ${AUDIOLIB_SOURCES})
set_property(TARGET audio_lib PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

##
target_include_directories(audio_lib
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/speex/include
    ../../libs
    ../)
