project (media_lib)

# Rely on C++ 11
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB MEDIA_LIB_SOURCES "*.cpp" "*.h")

add_library(media_lib ${MEDIA_LIB_SOURCES})
target_include_directories(media_lib PRIVATE ../../libs/ ../ ../../libs/srtp/include ../../libs/srtp/crypto/include)
