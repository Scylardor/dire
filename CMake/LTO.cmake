function(ENABLE_IPO_FOR Target Config)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_result OUTPUT ipo_err)
    if(ipo_result)
        message(STATUS "IPO is supported, enabling it for target ${Target} on config ${Config}.")
        set_property(
            TARGET ${Target}
            PROPERTY INTERPROCEDURAL_OPTIMIZATION_${Config} TRUE
        )
    else()
        message(STATUS "IPO is not supported.")
    endif()
endfunction()
