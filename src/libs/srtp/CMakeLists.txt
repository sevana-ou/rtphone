project (srtp)

# Rely on C++ 11
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set (SRTP_SOURCES
    srtp/srtp.c
    srtp/ekt.c
    crypto/rng/rand_source.c
    crypto/rng/prng.c
    crypto/rng/ctr_prng.c
    crypto/replay/ut_sim.c
    crypto/replay/rdbx.c
    crypto/replay/rdb.c
    crypto/math/stat.c
    crypto/math/gf2_8.c
    crypto/kernel/key.c
    crypto/kernel/err.c
    crypto/kernel/crypto_kernel.c
    crypto/kernel/alloc.c
    crypto/hash/sha1.c
    crypto/hash/null_auth.c
    crypto/hash/hmac.c
    crypto/cipher/null_cipher.c
    crypto/cipher/cipher.c
    crypto/cipher/aes.c
    crypto/cipher/aes_icm.c
    crypto/cipher/aes_cbc.c
    crypto/ae_xfm/xfm.c
    crypto/hash/srtp_auth.c
    crypto/math/datatypes.c
)

add_library(srtp ${SRTP_SOURCES})
target_include_directories(srtp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/crypto/include
    )

if (CMAKE_SYSTEM MATCHES "Linux*" OR CMAKE_SYSTEM MATCHES "Darwin*" OR ANDROID_ABI)
    set (PLATFORM_DEFINES HAVE_NETINET_IN_H)
else()
    set (PLATFORM_DEFINES HAVE_WINSOCK2_H)
endif()

if (ANDROID_ABI)
    if (CMAKE_ANDROID_ARCH_ABI MATCHES "arm.*")
        message ("ARM based processor found, defining CPU_RISC")
        set (PLATFORM_DEFINES ${PLATFORM_DEFINES} -DCPU_RISC)
    else()
        set (PLATFORM_DEFINES ${PLATFORM_DEFINES} -DCPU_CISC)
    endif()
else()
    set (PLATFORM_DEFINES ${PLATFORM_DEFINES} CPU_CISC)
endif()

target_compile_definitions(srtp PUBLIC -DHAVE_INTTYPES_H ${PLATFORM_DEFINES})
