project (helper_lib)

# Rely on C++ 11
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB HELPER_LIB_SOURCES "*.cpp" "*.h")

add_library(helper_lib ${HELPER_LIB_SOURCES})
target_include_directories(helper_lib PRIVATE ../../libs/ ../../engine ../)
