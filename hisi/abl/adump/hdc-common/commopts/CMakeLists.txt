target_sources(adcore PRIVATE     
    adx_comm_opt_manager.cpp
    hdc_comm_opt.cpp
)
if(${PRODUCT} STREQUAL "ascend310rc")
    target_sources(adump PRIVATE
        adx_comm_opt_manager.cpp
    ) 
elseif(${PRODUCT} STREQUAL "npuf10")
    target_sources(adump PRIVATE
        adx_comm_opt_manager.cpp
    )
else()
    target_sources(adump PRIVATE     
        adx_comm_opt_manager.cpp
        hdc_comm_opt.cpp
    )
endif()
