project (ice_stack)

# Rely on C++ 11
set (CMAKE_CXX_STANDARD 11)
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

if (TARGET_MUSL)
    add_definitions(-DTARGET_MUSL)
endif()

add_definitions(-DUSE_NATIVE_SMARTPTR -D_WINSOCK_DEPRECATED_NO_WARNINGS)

add_library(ice_stack ${ICE_STACK_SOURCES})
