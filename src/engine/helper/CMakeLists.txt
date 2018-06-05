project (helper_lib)

# Rely on C++ 11
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set (HELPER_LIB_SOURCES 
    HL_AsyncCommand.cpp
    HL_Calculator.cpp
    HL_CsvReader.cpp
    HL_Epoll.cpp
    HL_HepSupport.cpp
    HL_IuUP.cpp
    HL_Log.cpp
    HL_NetworkFrame.cpp
    HL_NetworkSocket.cpp
    HL_OsVersion.cpp
    HL_Pointer.cpp
    HL_Rtp.cpp
    HL_Singletone.cpp
    HL_SocketHeap.cpp
    HL_String.cpp
    HL_Sync.cpp
    HL_Usb.cpp
    HL_Uuid.cpp
    HL_VariantMap.cpp
)

add_library(helper_lib ${HELPER_LIB_SOURCES})
