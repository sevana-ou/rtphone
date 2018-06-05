project (speexdsp)

# Rely on C++ 11
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

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
