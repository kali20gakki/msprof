############################################################################
#    libadump.so (device)
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libadump

LOCAL_SRC_FILES := ../../hdc-common/commopts/adx_comm_opt_manager.cpp \
                   ../../hdc-common/common/memory_utils.cpp \
                   ../../hdc-common/component/dump/adx_dump_process.cpp \
                   ../../hdc-common/component/dump/adx_dump_record.cpp \
                   ../../hdc-common/common/file_utils.cpp \
                   ../../hdc-common/protocol/adx_msg_proto.cpp \
                   ../../hdc-common/common/string_utils.cpp \
                   ../../hdc-common/component/dump/adx_dump_soc_helper.cpp \
                   ../../external/adx_datadump_server_soc.cpp \
                   ../../hdc-common/device/adx_device.cpp \
                   ../../hdc-common/device/adx_sock_device.cpp \
                   ../../hdc-common/commopts/sock_api.cpp \
                   ../../hdc-common/commopts/sock_comm_opt.cpp \
                   ../../hdc-common/common/thread.cpp \
                   ../../hdc-common/ide_process_util.cpp \

LOCAL_SHARED_LIBRARIES := libc_sec libmmpa libslog

SOC_SRC_FILES := ../../hdc-common/component/dump/adx_dump_soc_api.cpp \

ifeq ($(product), lhisi)
LOCAL_CFLAGS += -DIDE_UNIFY_HOST_DEVICE
LOCAL_SRC_FILES += ${SOC_SRC_FILES}

else ifeq ($(rc), mini)
LOCAL_CFLAGS += -DIDE_UNIFY_HOST_DEVICE
LOCAL_SRC_FILES += ${SOC_SRC_FILES}

else ifeq ($(product), mdc)
LOCAL_CFLAGS += -DIDE_UNIFY_HOST_DEVICE
LOCAL_SRC_FILES += ${SOC_SRC_FILES}

else
LOCAL_SRC_FILES += ../../hdc-common/device/adx_dsmi.cpp \
                   ../../hdc-common/device/adx_device.cpp \
                   ../../hdc-common/device/adx_hdc_device.cpp \
                   ../../hdc-common/hdc_api.cpp \
                   ../../hdc-common/ide_hdc_interface.cpp \
                   ../../external/adx_api.cpp \
                   ../../hdc-common/component/dump/adx_dump_hdc_helper.cpp \
                   ../../hdc-common/component/dump/adx_dump_hdc_api.cpp \
                   ../../hdc-common/commopts/hdc_comm_opt.cpp

LOCAL_SHARED_LIBRARIES += libascend_hal
endif

LOCAL_C_INCLUDES := ${LOCAL_PATH}/../../../../../libc_sec/include \
                    ${LOCAL_PATH}/../../hdc-common \
                    ${LOCAL_PATH}/../../hdc-common/commopts \
                    ${LOCAL_PATH}/../../hdc-common/common \
                    ${LOCAL_PATH}/../../hdc-common/protocol \
                    ${LOCAL_PATH}/../../hdc-common/device \
                    ${LOCAL_PATH}/../../hdc-common/log \
                    ${LOCAL_PATH}/../../hdc-common/component \
                    ${LOCAL_PATH}/../../hdc-common/component/dump \
                    ${LOCAL_PATH}/../../external \
                    ${LOCAL_PATH}/../../../../../inc/driver \
                    ${LOCAL_PATH}/../../../../../inc/toolchain \
                    ${LOCAL_PATH}/../../../../../inc/mmpa \

LOCAL_CFLAGS := -Werror -std=c++11 -fstack-protector-strong -DADX_LIB -DOS_TYPE=0 #OS_TYPE LINUX=0 WIN=1
LOCAL_LDFLAGS += -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack

include $(BUILD_SHARED_LIBRARY)
