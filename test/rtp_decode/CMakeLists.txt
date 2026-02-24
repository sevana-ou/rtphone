set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(rtp_decode main.cpp)
add_subdirectory(../../src build_rtphone)
target_link_libraries(rtp_decode PRIVATE rtphone)
