target_sources(adcore PRIVATE     
    hdc_api.cpp
    device/adx_dsmi.cpp
    device/adx_hdc_device.cpp
)

target_sources(adump PRIVATE     
    hdc_api.cpp
    device/adx_dsmi.cpp
)

add_subdirectory(component)
add_subdirectory(epoll)
add_subdirectory(common)
add_subdirectory(protocol)
add_subdirectory(commopts)
