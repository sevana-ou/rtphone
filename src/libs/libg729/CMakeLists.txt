project (g729)

# Rely on C++ 11
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set (G729_SOURCES 
    g729_acelp_ca.cpp
    g729_basic_op.cpp
    g729_cod_ld8a.cpp
    g729_cor_func.cpp
    g729_de_acelp.cpp
    g729_dec_gain.cpp
    g729_dec_lag3.cpp
    g729_dec_ld8a.cpp
    g729_dspfunc.cpp 
    g729_filter.cpp  
    g729_gainpred.cpp
    g729_lpc_729.cpp 
    g729_lpcfunc.cpp 
    g729_lspdec.cpp  
    g729_lspgetq.cpp 
    g729_oper_32b.cpp
    g729_pitch_a.cpp 
    g729_postfilt.cpp
    g729_post_pro.cpp
    g729_p_parity.cpp
    g729_pred_lt3.cpp
    g729_pre_proc.cpp
    g729_gain.cpp    
    g729_lsp.cpp     
    g729_tab_ld8a.cpp
    g729_taming.cpp  
    g729_util.cpp    
)

add_library(g729_codec ${G729_SOURCES})
