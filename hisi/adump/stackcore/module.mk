LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libstackcore

LOCAL_SRC_FILES := stackcore.c

LOCAL_C_INCLUDES := ${LOCAL_PATH}/../../../../libc_sec/include \
                    ${LOCAL_PATH}/../../../../inc/toolchain/ \
                    ${LOCAL_PATH}/../../../../inc/toolchain/stackcore \

LOCAL_CFLAGS :=-Werror -std=c99 -fstack-protector-strong -D_GNU_SOURCE

LOCAL_LDFLAGS += -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack

LOCAL_SHARED_LIBRARIES := libc_sec

include $(BUILD_SHARED_LIBRARY)

#####################################
# add stackview.sh
include $(CLEAR_VARS)

LOCAL_MODULE := stackview.sh
LOCAL_SRC_FILES := stackview.sh

LOCAL_MODULE_CLASS := ETC

LOCAL_INSTALLED_PATH := $(HOST_OUT_ROOT)/stackview.sh
include $(BUILD_HOST_PREBUILT)
