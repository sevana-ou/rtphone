project (evscodec)

# Rely on C++ 11
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

file(GLOB_RECURSE EVS_SOURCES "*.cpp" "*.h")

add_library(evs_codec ${EVS_SOURCES})
target_include_directories(evs_codec PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/basic_op
                                             ${CMAKE_CURRENT_SOURCE_DIR}/basic_math
                                             ${CMAKE_CURRENT_SOURCE_DIR}/lib_enc
                                             ${CMAKE_CURRENT_SOURCE_DIR}/lib_dec
                                             ${CMAKE_CURRENT_SOURCE_DIR}/lib_com)

set_property(TARGET evs_codec PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
