LOCAL_PATH := $(call my-dir)

libslog_src_files := ../slog.cpp \
                     ../slog_common.c \
                     ../../shared/cfg_file_parse.c \
                     ../../shared/print_log.c \
                     ../../shared/log_level_parse.cpp \
                     ../../shared/log_common_util.c \
                     ../../shared/msg_queue.c \
                     ../../shared/share_mem.c \
                     ../../shared/log_sys_package.c

libslog_inc_files := ${LOCAL_PATH}/../../../../abl/libc_sec/include \
                     ${LOCAL_PATH}/../../shared \
                     inc/toolchain

iam_libslog_src_files := ../slog_iam.cpp \
                         ../slog_common.c \
                         ../../shared/msg_queue.c \
                         ../../shared/cfg_file_parse.c \
                         ../../shared/print_log.c \
                         ../../shared/log_level_parse.cpp \
                         ../../shared/log_common_util.c \
                         ../../shared/share_mem.c \
                         ../../shared/log_sys_package.c

iam_libslog_inc_files := ${LOCAL_PATH}/../../../../abl/libc_sec/include \
                         ${LOCAL_PATH}/../../shared \
                         inc/toolchain \
                         ${MDC_LINUX_SDK_DIR}/include

####################################
# build libslog.so device
include $(CLEAR_VARS)
LOCAL_MODULE := libslog

ifeq ($(product)_$(chip_id), mdc_hi1951)
LOCAL_SRC_FILES := $(iam_libslog_src_files)
LOCAL_C_INCLUDES := $(iam_libslog_inc_files)
LOCAL_LDFLAGS += -L${MDC_LINUX_SDK_DIR}/lib64 \
                 -L${MDC_LINUX_SDK_DIR}/lib64/mdc/base-plat \
                 -liam -leasy_comm -lxshmem
else
LOCAL_SRC_FILES := $(libslog_src_files)
LOCAL_C_INCLUDES := $(libslog_inc_files)
endif

ifeq (${product}, cloud)
    LOCAL_CFLAGS += -D_DEV_CLOUD_
endif

LOCAL_CFLAGS += -DWRITE_TO_SYSLOG

ifeq ($(product)_$(rc), mini_mini)
    LOCAL_CFLAGS +=  -D_MINI_RC
endif
ifeq ($(device_os),android)
LOCAL_CFLAGS += -fstack-protector-strong -fPIE -D_FORTIFY_SOURCE=2 -O2 -DOS_TYPE_DEF=0
LOCAL_LDFLAGS += -Wl,-z,relro -Wl,-z,noexecstack -Wl,-z,now
else
LOCAL_CFLAGS += -Werror -fstack-protector-strong -fPIE -pie -D_FORTIFY_SOURCE=2 -O2 -DOS_TYPE_DEF=0
LOCAL_LDFLAGS += -lpthread -Wl,-z,relro -Wl,-z,noexecstack -Wl,-z,now
endif

LOCAL_SHARED_LIBRARIES += libc_sec
include $(BUILD_SHARED_LIBRARY)

####################################
# build libalog.so device
include $(CLEAR_VARS)
LOCAL_MODULE := libalog

ifeq ($(product)_$(chip_id), mdc_hi1951)
LOCAL_SRC_FILES := $(iam_libslog_src_files)
LOCAL_C_INCLUDES := $(iam_libslog_inc_files)
LOCAL_LDFLAGS += -L${MDC_LINUX_SDK_DIR}/lib64 \
                 -L${MDC_LINUX_SDK_DIR}/lib64/mdc/base-plat \
                 -liam -leasy_comm -lxshmem
else
LOCAL_SRC_FILES := $(libslog_src_files)
LOCAL_C_INCLUDES := $(libslog_inc_files)
endif

ifeq (${product}, cloud)
    LOCAL_CFLAGS += -D_DEV_CLOUD_
endif

LOCAL_CFLAGS += -DWRITE_TO_SYSLOG -DLOG_CPP

ifeq ($(product)_$(rc), mini_mini)
    LOCAL_CFLAGS +=  -D_MINI_RC
endif
ifeq ($(device_os),android)
LOCAL_CFLAGS += -fstack-protector-strong -fPIE -D_FORTIFY_SOURCE=2 -O2 -DOS_TYPE_DEF=0
LOCAL_LDFLAGS += -Wl,-z,relro -Wl,-z,noexecstack -Wl,-z,now
else
LOCAL_CFLAGS += -Werror -fstack-protector-strong -fPIE -pie -D_FORTIFY_SOURCE=2 -O2 -DOS_TYPE_DEF=0
LOCAL_LDFLAGS += -lpthread -Wl,-z,relro -Wl,-z,noexecstack -Wl,-z,now
endif

LOCAL_SHARED_LIBRARIES += libc_sec
include $(BUILD_SHARED_LIBRARY)

####################################
# build libslog.a device
include $(CLEAR_VARS)
LOCAL_MODULE := libslog

ifeq ($(product)_$(chip_id), mdc_hi1951)
LOCAL_SRC_FILES := $(iam_libslog_src_files)
LOCAL_C_INCLUDES := $(iam_libslog_inc_files)
LOCAL_LDFLAGS += -L${MDC_LINUX_SDK_DIR}/lib64 \
                 -L${MDC_LINUX_SDK_DIR}/lib64/mdc/base-plat \
                 -liam -leasy_comm -lxshmem
else
LOCAL_SRC_FILES := $(libslog_src_files)
LOCAL_C_INCLUDES := $(libslog_inc_files)
endif

ifeq (${product}, cloud)
   LOCAL_CFLAGS += -D_DEV_CLOUD_
endif

ifeq ($(product)_$(rc), mini_mini)
    LOCAL_CFLAGS +=  -D_MINI_RC
endif
ifeq ($(device_os),android)
LOCAL_CFLAGS += -fstack-protector-strong -fPIE -D_FORTIFY_SOURCE=2 -O2 -DOS_TYPE_DEF=0
LOCAL_LDFLAGS += -Wl,-z,relro -Wl,-z,noexecstack -Wl,-z,now
else
LOCAL_CFLAGS += -Werror -fstack-protector-strong -fPIE -pie -D_FORTIFY_SOURCE=2 -O2 -DOS_TYPE_DEF=0
LOCAL_LDFLAGS += -lpthread -Wl,-z,relro -Wl,-z,noexecstack -Wl,-z,now
endif

LOCAL_STATIC_LIBRARIES := libc_sec
include $(BUILD_STATIC_LIBRARY)
