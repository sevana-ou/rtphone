project (speexdsp)

# Rely on C++ 11
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set (SPEEXDSP_SOURCES
    libspeexdsp/buffer.c
    libspeexdsp/fftwrap.c
    libspeexdsp/filterbank.c
#   libspeexdsp/kiss_fft.c
#   libspeexdsp/kiss_fftr.c
    libspeexdsp/mdf.c
    libspeexdsp/scal.c
    libspeexdsp/smallft.c
    libspeexdsp/speex_resample.c
    libspeexdsp/speex_preprocess.c
)

add_library(speexdsp ${SPEEXDSP_SOURCES})
target_compile_definitions(speexdsp PUBLIC -DUSE_KISS_FFT -DFLOATING_POINT -DHAVE_STDINT_H)

target_include_directories(speexdsp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/libspeexdsp
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
