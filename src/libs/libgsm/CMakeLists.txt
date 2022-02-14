project (gsm_codec)

# Rely on C++ 11
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set (GSM_SOURCES
    gsm_add.c          
    gsm_code.c         
    gsm_decode_helper.c
    gsm_create.c       
    gsm_decode.c       
    gsm_destroy.c      
    gsm_encode.c       
    gsm_lpc.c          
    gsm_option.c       
    gsm_long_term.c    
    gsm_rpe.c          
    gsm_short_term.c   
    gsm_table.c        
    gsm_preprocess.c   
)

add_library(gsm_codec ${GSM_SOURCES})
target_include_directories(gsm_codec PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(gsm_codec PUBLIC HAS_STDLIB_H HAS_STRING_H)
set_property(TARGET gsm_codec PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
