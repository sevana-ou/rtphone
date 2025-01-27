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
    ICETransactionList.cpp
    hmac_sha1_impl.h
    hmac_sha1_impl.cpp
    md5_impl.h
    md5_impl.cpp
)

set (ICE_DEFINES -D_WINSOCK_DEPRECATED_NO_WARNINGS)
if (TARGET_MUSL)
    set(ICE_DEFINES ${ICE_DEFINES} -DTARGET_MUSL)
endif()

add_library(ice_stack ${ICE_STACK_SOURCES})
target_compile_definitions(ice_stack PRIVATE ${ICE_DEFINES})
set_target_properties(ice_stack PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
