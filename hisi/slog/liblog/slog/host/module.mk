LOCAL_PATH := $(call my-dir)

libslog_src_files := ../slog.cpp \
                     ../slog_common.c \
                     ../../shared/cfg_file_parse.c \
                     ../../shared/print_log.c \
                     ../../shared/log_level_parse.cpp\
                     ../../shared/log_common_util.c \
                     ../../shared/msg_queue.c \
                     ../../shared/share_mem.c \
                     ../../shared/log_sys_package.c

libslog_inc_files := ${LOCAL_PATH}/../../../../abl/libc_sec/include \
                     ${LOCAL_PATH}/../../shared \
                     inc/toolchain

####################################
# build libslog.so host
ifneq (${product}, mdc)

include $(CLEAR_VARS)
LOCAL_MODULE := libslog

LOCAL_SRC_FILES := $(libslog_src_files)

LOCAL_C_INCLUDES := $(libslog_inc_files)

ifeq (${product},cloud)
    scene_cloud := -D_HOST_CLOUD
endif

ifneq ($(product)_$(chip_id), mdc_hi1951)
    LOCAL_CFLAGS += -DWRITE_TO_SYSLOG
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

LOCAL_CFLAGS += -Werror -fPIE -pie -D_FORTIFY_SOURCE=2 -O2 -DOS_TYPE_DEF=0  ${scene_cloud}
LOCAL_LDFLAGS := -lpthread -lc_sec -Wl,-z,relro -Wl,-z,noexecstack -Wl,-z,now
LOCAL_SHARED_LIBRARIES := libc_sec
include $(BUILD_HOST_SHARED_LIBRARY)

####################################
# build libalog.so host
include $(CLEAR_VARS)
LOCAL_MODULE := libalog

LOCAL_SRC_FILES := $(libslog_src_files)

LOCAL_C_INCLUDES := $(libslog_inc_files)

ifeq (${product},cloud)
    scene_cloud := -D_HOST_CLOUD
endif

ifneq ($(product)_$(chip_id), mdc_hi1951)
    LOCAL_CFLAGS += -DWRITE_TO_SYSLOG
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

LOCAL_CFLAGS += -Werror -fPIE -pie -D_FORTIFY_SOURCE=2 -O2 -DOS_TYPE_DEF=0  ${scene_cloud} -DLOG_CPP -DPROCESS_LOG
LOCAL_LDFLAGS := -lpthread -lc_sec -Wl,-z,relro -Wl,-z,noexecstack -Wl,-z,now -Lout/${product}/host/obj/STATIC_LIBRARIES/libplog_intermediates -Wl,--whole-archive -lplog -Wl,--no-whole-archive -ldl
LOCAL_SHARED_LIBRARIES := libc_sec
LOCAL_STATIC_LIBRARIES := libplog
include $(BUILD_HOST_SHARED_LIBRARY)

endif
####################################
# build libslog.a host
include $(CLEAR_VARS)
LOCAL_MODULE := libslog

LOCAL_SRC_FILES := $(libslog_src_files)

LOCAL_C_INCLUDES := $(libslog_inc_files)

ifeq (${product},cloud)
   scene_cloud := -D_HOST_CLOUD
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
LOCAL_CFLAGS := -fstack-protector-strong
else
LOCAL_CFLAGS := -fstack-protector-all
endif

LOCAL_CFLAGS += -Werror -fPIE -pie -D_FORTIFY_SOURCE=2 -O2 -DOS_TYPE_DEF=0  ${scene_cloud}
LOCAL_LDFLAGS := -lpthread -Wl,-z,relro -Wl,-z,noexecstack -Wl,-z,now

# build libslog.a
LOCAL_SHARED_LIBRARIES :=
LOCAL_STATIC_LIBRARIES := libc_sec

include $(BUILD_HOST_STATIC_LIBRARY)
####################################
# build libslog.so for llt
include $(CLEAR_VARS)
LOCAL_MODULE := libslog

LOCAL_SRC_FILES := $(libslog_src_files)

LOCAL_C_INCLUDES := $(libslog_inc_files)

LOCAL_CFLAGS := -Werror -D__IDE_UT
LOCAL_LDFLAGS := -lpthread -lc_sec
LOCAL_SHARED_LIBRARIES := libc_sec
include $(BUILD_LLT_SHARED_LIBRARY)

######################################
# add slog.h to host for mdc
include $(CLEAR_VARS)

LOCAL_MODULE := slog_headers
LOCAL_SRC_FILES := ../../../../inc/toolchain/slog.h

LOCAL_MODULE_CLASS := ETC

LOCAL_INSTALLED_PATH := $(HOST_OUT_ROOT)/slogheader/slog.h
include $(BUILD_HOST_PREBUILT)
