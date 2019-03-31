project (audio_lib)

# Rely on C++ 11
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set (AUDIOLIB_SOURCES
    Audio_Resampler.cpp
    Audio_Quality.cpp
    Audio_Mixer.cpp
    Audio_Interface.cpp
    Audio_Helper.cpp
    Audio_DataWindow.cpp
    Audio_DevicePair.cpp
    Audio_Player.cpp
    Audio_Null.cpp
    Audio_CoreAudio.cpp
    Audio_DirectSound.cpp
    Audio_WavFile.cpp
)

add_library(audio_lib ${AUDIOLIB_SOURCES})

target_include_directories(audio_lib
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE ../../libs/speex/include ../../libs ../)
