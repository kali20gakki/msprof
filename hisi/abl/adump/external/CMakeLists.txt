target_sources(adcore PRIVATE     
    adx_core_dump_server.cpp
)
if(${PRODUCT} STREQUAL "ascend310rc")
elseif(${PRODUCT} STREQUAL "npuf10")
else()
    target_sources(adump PRIVATE     
        adx_api.cpp
    )
endif()
