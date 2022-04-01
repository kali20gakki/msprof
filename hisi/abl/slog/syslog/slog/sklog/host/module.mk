LOCAL_PATH := $(call my-dir)

# include print_log.c to avoid make sklog.log old fail and fail to change sklogd to HwHiAiUser
sklogd_src_files := ../src/klogd.c \
                    ../../common/common.c \
                    ../../../shared/start_single_process.c \
                    ../../../shared/log_sys_package.c \
                    ../../../shared/print_log.c \
                    ../../slog/src/log_monitor.c

sklogd_inc_files := ${LOCAL_PATH}/../../../../../abl/libc_sec/include ${LOCAL_PATH}/../../include \
                    ${LOCAL_PATH}/../ \
                    ${LOCAL_PATH}/../../common \
                    ${LOCAL_PATH}/../../../shared \
                    ${LOCAL_PATH}/../../slog/src \
                    inc/driver \
                    inc/toolchain


#####################################
# build sklogd host
ifneq (${product}, mdc)
ifneq (${product}, mini)

include $(CLEAR_VARS)
LOCAL_MODULE := sklogd

LOCAL_SRC_FILES := $(sklogd_src_files)

LOCAL_C_INCLUDES := $(sklogd_inc_files)

ifneq (,$(wildcard $(PWD)/$($(HOST_DEFAULT_COMPILER)_TOOLS_PREFIX)gcc))
CROSS_COMPILE_PREFIX := $(PWD)/$($(HOST_DEFAULT_COMPILER)_TOOLS_PREFIX)
else
CROSS_COMPILE_PREFIX := $($(HOST_DEFAULT_COMPILER)_TOOLS_PREFIX)
endif
GCC_VERSION := $(shell ($(CROSS_COMPILE_PREFIX)gcc --version | head -n1 | cut -d" " -f4))
GCC_VERSION_FLAG := \
$(shell if [[ "$(GCC_VERSION)" > "4.9" ]];then echo 1; else echo 0; fi)

ifeq ($(GCC_VERSION_FLAG), 1)
LOCAL_CFLAGS := -fstack-protector-strong
else
LOCAL_CFLAGS := -fstack-protector-all
endif

ifeq ($(product), cloud)
    LOCAL_CFLAGS += -D_LOG_MONITOR_ -D_HOST_CLOUD
    LOCAL_SHARED_LIBRARIES += libascend_monitor
else ifeq ($(product), mini)
    ifneq ($(rc), mini)
        LOCAL_CFLAGS += -D_LOG_MONITOR_
        LOCAL_SHARED_LIBRARIES += libascend_monitor
    endif
endif

LOCAL_CFLAGS += -Werror -fPIE -pie -D_FORTIFY_SOURCE=2 -O2 -DOS_TYPE_DEF=0

LOCAL_LDFLAGS := -lpthread -lc_sec -Wl,-z,relro -Wl,-z,noexecstack -fPIE -pie -Wl,-z,now
LOCAL_SHARED_LIBRARIES += libc_sec libslog
include $(BUILD_HOST_EXECUTABLE)

endif
endif
######################################
# add sklogd_daemon_monitor.sh to host
include $(CLEAR_VARS)

LOCAL_MODULE := sklogd_daemon_monitor.sh
LOCAL_SRC_FILES := ../../../scripts/sklogd_daemon_monitor.sh

LOCAL_MODULE_CLASS := ETC

LOCAL_INSTALLED_PATH := $(HOST_OUT_ROOT)/sklogd_daemon_monitor.sh
include $(BUILD_HOST_PREBUILT)
