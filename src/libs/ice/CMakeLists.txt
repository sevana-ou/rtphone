project (ice_stack)

# Rely on C++ 17
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set (ICE_STACK_SOURCES ICEAddress.cpp
    ICEAuthTransaction.cpp
    ICEBinding.cpp
    ICEBox.cpp
    ICEBoxImpl.cpp
    ICEByteBuffer.cpp
    ICECandidate.cpp
    ICECandidatePair.cpp
    ICECheckList.cpp
    ICECRC32.cpp
    ICEError.cpp
    ICELog.cpp
    ICEMD5.cpp
    ICENetworkHelper.cpp
    ICEPacketTimer.cpp
    ICEPlatform.cpp
    ICERelaying.cpp
    ICESession.cpp
    ICESHA1.cpp
    ICEStream.cpp
    ICEStunAttributes.cpp
    ICEStunConfig.cpp
    ICEStunMessage.cpp
    ICEStunTransaction.cpp
    ICESync.cpp
    ICETime.cpp
    ICETransactionList.cpp)


#if (ANDROID_ABI)
#    set (ICE_STACK_SOURCES ${ICE_STACK_SOURCES} android-ifaddrs/android-ifaddrs.h android-ifaddrs/android-ifaddrs.c)
#endif()

if (TARGET_MUSL)
    add_definitions(-DTARGET_MUSL)
endif()

add_definitions(-DUSE_NATIVE_SMARTPTR -D_WINSOCK_DEPRECATED_NO_WARNINGS -DUSE_OPENSSL)

add_library(ice_stack ${ICE_STACK_SOURCES})
set_property(TARGET ice_stack PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_link_libraries(ice_stack PUBLIC ssl crypto)
