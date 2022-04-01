set(SECUREC_DIR ${TOP_DIR}/hisi/securec)
set(MMPA_DIR ${TOP_DIR}/hisi/mmpa)
set(SLOG_DIR ${TOP_DIR}/hisi/slog/liblog)

include(${TOP_DIR}/cmake/install_securec.cmake)

# include(install_ge_executanle.cmake)
# include(install_ascend_hal.cmake)
# include(install_runtime.cmake)
# include(install_error_manager.cmake)
include(${TOP_DIR}/cmake/install_mmpa.cmake)
include(${TOP_DIR}/cmake/install_slog.cmake)
