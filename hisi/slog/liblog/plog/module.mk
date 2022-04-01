LOCAL_PATH := $(call my-dir)

log_root_path=toolchain/log

plog_src_files := process_log.c \
                   ../../shared/log_to_file.c \
                   ../../external/log_drv.c \
                   ../../external/log_runtime.c \
                   ../../external/api/driver_api.c \
                   ../../shared/library_load.c

plog_inc_files := abl/libc_sec/include \
                    inc/driver \
                    inc/toolchain \
                    inc/mmpa \
                    toolchain/log/external \
                    toolchain/log/shared \
                    toolchain/log/liblog/slog

####################################
# build libplog static library
include $(CLEAR_VARS)

LOCAL_MODULE := libplog
LOCAL_SRC_FILES := $(plog_src_files)
LOCAL_C_INCLUDES := $(plog_inc_files)

ifeq (${product}, cloud)
  LOCAL_CFLAGS += -DLARGE_LOGFILE_VOLUME
else ifeq ($(product)_$(chip_id), mini_hi1951)
  LOCAL_CFLAGS += -DLARGE_LOGFILE_VOLUME
else ifeq ($(product), mdc)
  LOCAL_CFLAGS += -DLARGE_LOGFILE_NUM
else ifeq ($(product), mini)
  ifneq ($(rc), mini)
    LOCAL_CFLAGS += -DLARGE_LOGFILE_VOLUME
  endif
endif

ifneq (,$(wildcard $(PWD)/$($(HOST_DEFAULT_COMPILER)_TOOLS_PREFIX)gcc))
CROSS_COMPILE_PREFIX := $(PWD)/$($(HOST_DEFAULT_COMPILER)_TOOLS_PREFIX)
else
CROSS_COMPILE_PREFIX := $($(HOST_DEFAULT_COMPILER)_TOOLS_PREFIX)
endif
GCC_VERSION := $(shell ($(CROSS_COMPILE_PREFIX)gcc --version | head -n1 | cut -d" " -f4))
GCC_VERSION_FLAG := \
$(shell if [[ "$(GCC_VERSION)" > "4.9" ]];then echo 1; else echo 0; fi)

ifeq ($(GCC_VERSION_FLAG), 1)
LOCAL_CFLAGS += -fstack-protector-strong
else
LOCAL_CFLAGS += -fstack-protector-all
endif

LOCAL_CFLAGS += -Werror -fPIE -pie -D_FORTIFY_SOURCE=2 -O2 -D_NONE_ZIP_ -DPROCESS_LOG -DWRITE_TO_SYSLOG -DOS_TYPE_DEF=0
LOCAL_LDFLAGS := -lpthread -lc_sec -Wl,-z,relro -Wl,-z,noexecstack -fPIE -pie -Wl,-z,now -ldl
LOCAL_SHARED_LIBRARIES += libc_sec libmmpa
include $(BUILD_HOST_STATIC_LIBRARY)
