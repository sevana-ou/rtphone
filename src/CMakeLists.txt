cmake_minimum_required(VERSION 3.20)
project(rtphone)

# Rely on C++ 23
set (CMAKE_CXX_STANDARD 23)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set (L libs)
set (E engine)

option (USE_AMR_CODEC   "Use AMR codec. Requires libraries."    ON)
option (USE_EVS_CODEC   "Use EVS codec."                        ON)
option (USE_OPUS_CODEC  "Use Opus codec."                       ON)
option (USE_MUSL        "Build with MUSL library"               OFF)

# PIC code by default
set (CMAKE_POSITION_INDEPENDENT_CODE ON)
set (RUNTIME_CPU_CAPABILITY_DETECTION ON)

set (LIB_PLATFORM ${CMAKE_CURRENT_SOURCE_DIR}/libs/libraries)
include (${LIB_PLATFORM}/platform_libs.cmake)


message("Libraries: ${LIB_PLATFORM}")
set (OPENSSL_INCLUDE ${LIB_PLATFORM}/openssl/1.1/include)
message ("Using OpenSSL include files from ${OPENSSL_INCLUDE}")
message ("Using OpenSSL libs: ${OPENSSL_SSL} and ${OPENSSL_CRYPTO}")
include_directories(${OPENSSL_INCLUDE})

# Used defines for our project
set (DEFINES -DUSE_OPENSSL)

# Libraries for our project
set (LIBS "")

# Windows-specific definitions
if (CMAKE_SYSTEM MATCHES "Windows*")
    set (DEFINES ${DEFINES} -DTARGET_WIN -D_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS -D_UNICODE -D_CRT_SECURE_NO_WARNINGS)
    set (TARGET_WIN ON)
endif()

# Linux-specific definitions
if (CMAKE_SYSTEM MATCHES "Linux*")
    set (DEFINES ${DEFINES} -DTARGET_LINUX -DHAVE_NETINET_IN_H)
    set (TARGET_LINUX ON)
    set (LIBS ${LIBS} dl uuid)
endif()

# macOS-specific definitions
if (CMAKE_SYSTEM MATCHES "Darwin*")
    set (DEFINES ${DEFINES} -DTARGET_OSX)
    set (TARGET_OSX ON)
    set (LIBS ${LIBS} dl uuid)
endif()

# 
if (CMAKE_SYSTEM MATCHES "Android")
    message("Adding the Oboe library")
    set (OBOE_DIR libs/oboe)
    add_subdirectory (${OBOE_DIR} ./oboe)
    include_directories (${OBOE_DIR}/include)
    set (DEFINES ${DEFINES} -DTARGET_ANDROID -DHAVE_NETINET_IN_H)
    set (TARGET_ANDROID ON)
    set (LIBS ${LIBS} oboe)
endif()

if (USE_MUSL)
    set (DEFINES ${DEFINES} -DTARGET_MUSL)
    set (TARGET_MUSL ON)
endif()

set (RTPHONE_SOURCES
    ${E}/engine_config.h
    ${E}/media/MT_Statistics.cpp
    ${E}/media/MT_WebRtc.cpp
    ${E}/media/MT_Stream.cpp
    ${E}/media/MT_SrtpHelper.cpp
    ${E}/media/MT_SingleAudioStream.cpp
    ${E}/media/MT_NativeRtpSender.cpp
    ${E}/media/MT_Dtmf.cpp
    ${E}/media/MT_CodecList.cpp
    ${E}/media/MT_Codec.cpp
    ${E}/media/MT_Box.cpp
    ${E}/media/MT_AudioStream.cpp
    ${E}/media/MT_AudioReceiver.cpp
    ${E}/media/MT_AudioCodec.cpp
    ${E}/media/MT_CngHelper.cpp
    ${E}/agent/Agent_Impl.cpp
    ${E}/agent/Agent_Impl.h
    ${E}/agent/Agent_AudioManager.cpp
    ${E}/agent/Agent_AudioManager.h
    ${E}/endpoint/EP_Account.cpp
    ${E}/endpoint/EP_Account.h
    ${E}/endpoint/EP_AudioProvider.cpp
    ${E}/endpoint/EP_AudioProvider.h
    ${E}/endpoint/EP_DataProvider.cpp
    ${E}/endpoint/EP_DataProvider.h
    ${E}/endpoint/EP_Engine.cpp
    ${E}/endpoint/EP_Engine.h
    ${E}/endpoint/EP_NetworkQueue.cpp
    ${E}/endpoint/EP_NetworkQueue.h
    ${E}/endpoint/EP_Observer.cpp
    ${E}/endpoint/EP_Observer.h
    ${E}/endpoint/EP_Session.cpp
    ${E}/endpoint/EP_Session.h

    ${E}/media/MT_Statistics.h
    ${E}/media/MT_WebRtc.h
    ${E}/media/MT_Stream.h
    ${E}/media/MT_SrtpHelper.h
    ${E}/media/MT_SingleAudioStream.h
    ${E}/media/MT_NativeRtpSender.h
    ${E}/media/MT_Dtmf.h
    ${E}/media/MT_CodecList.h
    ${E}/media/MT_Codec.h
    ${E}/media/MT_Box.h
    ${E}/media/MT_AudioStream.h
    ${E}/media/MT_AudioReceiver.h
    ${E}/media/MT_AudioCodec.h
    ${E}/media/MT_CngHelper.h
    
    ${E}/media/MT_Statistics.cpp
    ${E}/media/MT_WebRtc.cpp
    ${E}/media/MT_Stream.cpp
    ${E}/media/MT_SrtpHelper.cpp
    ${E}/media/MT_SingleAudioStream.cpp
    ${E}/media/MT_NativeRtpSender.cpp
    ${E}/media/MT_Dtmf.cpp
    ${E}/media/MT_CodecList.cpp
    ${E}/media/MT_Codec.cpp
    ${E}/media/MT_Box.cpp
    ${E}/media/MT_AudioStream.cpp
    ${E}/media/MT_AudioReceiver.cpp
    ${E}/media/MT_AudioCodec.cpp
    ${E}/media/MT_CngHelper.cpp
    ${E}/media/MT_AmrCodec.cpp
    ${E}/media/MT_EvsCodec.cpp
    ${E}/media/MT_Statistics.h
    ${E}/media/MT_WebRtc.h
    ${E}/media/MT_Stream.h
    ${E}/media/MT_SrtpHelper.h
    ${E}/media/MT_SingleAudioStream.h
    ${E}/media/MT_NativeRtpSender.h
    ${E}/media/MT_Dtmf.h
    ${E}/media/MT_CodecList.h
    ${E}/media/MT_Codec.h
    ${E}/media/MT_Box.h
    ${E}/media/MT_AudioStream.h
    ${E}/media/MT_AudioReceiver.h
    ${E}/media/MT_AudioCodec.h
    ${E}/media/MT_CngHelper.h
    ${E}/media/MT_AmrCodec.h
    ${E}/media/MT_EvsCodec.h

    ${E}/helper/HL_AsyncCommand.cpp
    ${E}/helper/HL_AsyncCommand.h
    ${E}/helper/HL_Base64.h
    ${E}/helper/HL_ByteBuffer.h
    ${E}/helper/HL_Calculator.cpp
    ${E}/helper/HL_Calculator.h
    ${E}/helper/HL_CrashRpt.cpp
    ${E}/helper/HL_CrashRpt.h
    ${E}/helper/HL_CsvReader.cpp
    ${E}/helper/HL_CsvReader.h
    ${E}/helper/HL_Epoll.cpp
    ${E}/helper/HL_Epoll.h
    ${E}/helper/HL_Exception.h
    ${E}/helper/HL_File.cpp
    ${E}/helper/HL_File.h
    ${E}/helper/HL_HepSupport.cpp
    ${E}/helper/HL_HepSupport.h
    ${E}/helper/HL_InternetAddress.h
    ${E}/helper/HL_IuUP.cpp
    ${E}/helper/HL_IuUP.h
    ${E}/helper/HL_Log.cpp
    ${E}/helper/HL_Log.h
    ${E}/helper/HL_NetworkFrame.cpp
    ${E}/helper/HL_NetworkFrame.h
    ${E}/helper/HL_NetworkSocket.cpp
    ${E}/helper/HL_NetworkSocket.h
    ${E}/helper/HL_Optional.hpp
    ${E}/helper/HL_OsVersion.cpp
    ${E}/helper/HL_OsVersion.h
    ${E}/helper/HL_Pointer.cpp
    ${E}/helper/HL_Pointer.h
    ${E}/helper/HL_Process.cpp
    ${E}/helper/HL_Process.h
    ${E}/helper/HL_Rtp.cpp
    ${E}/helper/HL_Rtp.h
    ${E}/helper/HL_Singletone.cpp
    ${E}/helper/HL_Singletone.h
    ${E}/helper/HL_SocketHeap.cpp
    ${E}/helper/HL_SocketHeap.h
    ${E}/helper/HL_Statistics.cpp
    ${E}/helper/HL_Statistics.h
    ${E}/helper/HL_StreamState.h
    ${E}/helper/HL_String.cpp
    ${E}/helper/HL_String.h
    ${E}/helper/HL_Sync.cpp
    ${E}/helper/HL_Sync.h
    ${E}/helper/HL_ThreadPool.cpp
    ${E}/helper/HL_ThreadPool.h
    ${E}/helper/HL_Time.cpp
    ${E}/helper/HL_Time.h
    ${E}/helper/HL_Types.h
    ${E}/helper/HL_Types.cpp
    ${E}/helper/HL_Usb.cpp
    ${E}/helper/HL_Usb.h
    ${E}/helper/HL_Uuid.cpp
    ${E}/helper/HL_Uuid.h
    ${E}/helper/HL_VariantMap.cpp
    ${E}/helper/HL_VariantMap.h
    ${E}/helper/HL_Xcap.cpp
    ${E}/helper/HL_Xcap.h

    ${E}/audio/Audio_Resampler.cpp
    ${E}/audio/Audio_Resampler.h
    ${E}/audio/Audio_Quality.cpp
    ${E}/audio/Audio_Quality.h
    ${E}/audio/Audio_Mixer.cpp
    ${E}/audio/Audio_Mixer.h
    ${E}/audio/Audio_Interface.cpp
    ${E}/audio/Audio_Interface.h
    ${E}/audio/Audio_Helper.cpp
    ${E}/audio/Audio_Helper.h
    ${E}/audio/Audio_DataWindow.cpp
    ${E}/audio/Audio_DataWindow.h
    ${E}/audio/Audio_DevicePair.cpp
    ${E}/audio/Audio_DevicePair.h
    ${E}/audio/Audio_Player.cpp
    ${E}/audio/Audio_Player.h
    ${E}/audio/Audio_Null.cpp
    ${E}/audio/Audio_Null.h
    ${E}/audio/Audio_CoreAudio.cpp
    ${E}/audio/Audio_CoreAudio.h
    ${E}/audio/Audio_DirectSound.cpp
    ${E}/audio/Audio_DirectSound.h
    ${E}/audio/Audio_AndroidOboe.cpp
    ${E}/audio/Audio_AndroidOboe.h
    ${E}/audio/Audio_WavFile.cpp
    ${E}/audio/Audio_WavFile.h

    ${L}/ice/hmac_sha1_impl.cpp
    ${L}/ice/hmac_sha1_impl.h
    ${L}/ice/ICEAction.h
    ${L}/ice/ICEAddress.cpp
    ${L}/ice/ICEAddress.h
    ${L}/ice/ICEAuthTransaction.cpp
    ${L}/ice/ICEAuthTransaction.h
    ${L}/ice/ICEBinding.cpp
    ${L}/ice/ICEBinding.h
    ${L}/ice/ICEBox.cpp
    ${L}/ice/ICEBox.h
    ${L}/ice/ICEBoxImpl.cpp
    ${L}/ice/ICEBoxImpl.h
    ${L}/ice/ICEByteBuffer.cpp
    ${L}/ice/ICEByteBuffer.h
    ${L}/ice/ICECandidate.cpp
    ${L}/ice/ICECandidate.h
    ${L}/ice/ICECandidatePair.cpp
    ${L}/ice/ICECandidatePair.h
    ${L}/ice/ICECheckList.cpp
    ${L}/ice/ICECheckList.h
    ${L}/ice/ICECRC32.cpp
    ${L}/ice/ICECRC32.h
    ${L}/ice/ICEError.cpp
    ${L}/ice/ICEError.h
    ${L}/ice/ICEEvent.h
    ${L}/ice/ICELog.cpp
    ${L}/ice/ICELog.h
    ${L}/ice/ICEMD5.cpp
    ${L}/ice/ICEMD5.h
    ${L}/ice/ICENetworkHelper.cpp
    ${L}/ice/ICENetworkHelper.h
    ${L}/ice/ICEPacketTimer.cpp
    ${L}/ice/ICEPacketTimer.h
    ${L}/ice/ICEPlatform.cpp
    ${L}/ice/ICEPlatform.h
    ${L}/ice/ICERelaying.cpp
    ${L}/ice/ICERelaying.h
    ${L}/ice/ICESession.cpp
    ${L}/ice/ICESession.h
    ${L}/ice/ICESHA1.cpp
    ${L}/ice/ICESHA1.h
    ${L}/ice/ICESocket.h
    ${L}/ice/ICEStream.cpp
    ${L}/ice/ICEStream.h
    ${L}/ice/ICEStunAttributes.cpp
    ${L}/ice/ICEStunAttributes.h
    ${L}/ice/ICEStunConfig.cpp
    ${L}/ice/ICEStunConfig.h
    ${L}/ice/ICEStunMessage.cpp
    ${L}/ice/ICEStunMessage.h
    ${L}/ice/ICEStunTransaction.cpp
    ${L}/ice/ICEStunTransaction.h
    ${L}/ice/ICESync.cpp
    ${L}/ice/ICESync.h
    ${L}/ice/ICETime.cpp
    ${L}/ice/ICETime.h
    ${L}/ice/ICETransactionList.cpp
    ${L}/ice/ICETransactionList.h
    ${L}/ice/ICETypes.h
    ${L}/ice/md5_impl.cpp
    ${L}/ice/md5_impl.h
    )

set (DEFINES ${DEFINES} -DUSE_OPUS_CODEC)

add_library (rtphone STATIC ${RTPHONE_SOURCES})

add_subdirectory(${L}/resiprocate)
add_subdirectory(${L}/jrtplib/src)
add_subdirectory(${L}/libg729)

if (USE_EVS_CODEC)
    add_subdirectory(${L}/libevs)
endif()

add_subdirectory(${L}/libgsm)
add_subdirectory(${L}/gsmhr)
add_subdirectory(${L}/g722)
add_subdirectory(${L}/speexdsp)
add_subdirectory(${L}/libsrtp)
add_subdirectory(${L}/webrtc)

set (LIBS ${LIBS} jrtplib g729_codec gsm_codec opus
                  gsmhr_codec g722_codec srtp3 resiprocate webrtc speexdsp)

if (USE_AMR_CODEC)
    include (${LIB_PLATFORM}/platform_libs.cmake)
    message("Media: AMR NB and WB codecs will be included.")
    set (DEFINES ${DEFINES} -DUSE_AMR_CODEC)
    set (LIBS ${LIBS} ${OPENCORE_AMRNB} ${OPENCORE_AMRWB})
endif()

if (USE_EVS_CODEC)
    message("Media: EVS codec will be included.")
    set (DEFINES ${DEFINES} -DUSE_EVS_CODEC)
    set (LIBS ${LIBS} evs_codec)
endif()

target_compile_definitions(rtphone PUBLIC ${DEFINES})

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
        ${E}/helper
        ${E}/media
        ${L}
        ${L}/ice
    PRIVATE
        ${L}/libevs/lib_com
        ${L}/libevs/lib_enc
        ${L}/libevs/lib_dec
        ${L}/speex/include
        ${L}/libs/json
    )

# For MSVC static builds
set_property(TARGET rtphone PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
