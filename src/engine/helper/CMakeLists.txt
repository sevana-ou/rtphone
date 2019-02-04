project (helper_lib)

# Rely on C++ 11
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (USE_NULL_UUID OFF CACHE BOOL "When enabled linking to libuuid is avoided")

file(GLOB HELPER_LIB_SOURCES "*.cpp" "*.h")

add_library(helper_lib ${HELPER_LIB_SOURCES})
target_include_directories(helper_lib PRIVATE ../../libs/ ../../engine ../)
target_compile_definitions(helper_lib PRIVATE -D_CRT_SECURE_NO_WARNINGS)
