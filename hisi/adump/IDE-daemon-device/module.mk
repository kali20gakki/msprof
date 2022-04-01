LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

IDE_DAEMON_SRC_FILES := ../external/adx_utils.cpp \
                        ../hdc-common/ide_daemon.cpp \
                        ../hdc-common/ide_daemon_hdc.cpp \
                        ../hdc-common/hdc_api.cpp \
                        ../../hdc-common/ide_hdc_interface.cpp \
                        ../hdc-common/config/config_utils.cpp \
                        ../hdc-common/config/adx_config_manager.cpp \
                        ../hdc-common/ide_common_util.cpp \
                        ../hdc-common/ide_process_util.cpp \
                        ../hdc-common/common/thread.cpp \
                        ../hdc-common/ide_daemon_monitor.cpp \
                        ../hdc-common/ide_platform_util.cpp \
                        ../hdc-common/ide_task_register.cpp \
                        ../hdc-common/common/utils.cpp \
                        ../hdc-common/device/adx_dsmi.cpp \
                        ../hdc-common/common/file_utils.cpp \
                        ../hdc-common/common/string_utils.cpp \
                        ../hdc-common/common/memory_utils.cpp

IDE_DAEMON_INC_PATH :=  ${LOCAL_PATH}/../../../../libc_sec/include \
                        ${LOCAL_PATH}/../hdc-common \
                        ${LOCAL_PATH}/../hdc-common/common \
                        ${LOCAL_PATH}/../hdc-common/component/dump \
                        ${LOCAL_PATH}/../hdc-common/platform/include \
                        ${LOCAL_PATH}/../hdc-common/platform \
                        ${LOCAL_PATH} \
                        ${LOCAL_PATH}/../../../../inc/toolchain \
                        ${LOCAL_PATH}/../../../../inc/driver \
                        ${LOCAL_PATH}/../../../../inc/mmpa \
                        ${LOCAL_PATH}/../../../../third_party/protobuf/include \

LOCAL_MODULE := adda

LOCAL_SRC_FILES := ${IDE_DAEMON_SRC_FILES}

LOCAL_C_INCLUDES := ${IDE_DAEMON_INC_PATH}

LOCAL_CFLAGS := -Werror -DIDE_DAEMON_DEVICE -fstack-protector-strong -fPIE -std=c++11 -Dgoogle=ascend_private -DOS_TYPE=0 #OS_TYPE LINUX=0 WIN=1

LOCAL_LDFLAGS += -ldl -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -pie -s

LOCAL_SHARED_LIBRARIES := libc_sec libascend_hal libascend_protobuf libslog libmmpa

LOCAL_STATIC_LIBRARIES := libhdclog_device libdevprof libadcore

ifeq ($(rc), mini)
LOCAL_CFLAGS += -DIDE_DAEMON_RC
endif

ifeq ($(product), mini)
    LOCAL_SHARED_LIBRARIES += libstackcore libascend_monitor
    LOCAL_CFLAGS += -DPLATFORM_MONITOR
endif

ifeq ($(product), cloud)
LOCAL_CFLAGS += -DPLATFORM_MONITOR -DUSE_TRUSTED_BASE_PATH
LOCAL_SHARED_LIBRARIES += libascend_monitor libstackcore
endif

include $(BUILD_EXECUTABLE)

###############################################
# add ide_daemon.cfg to device
include $(CLEAR_VARS)

LOCAL_MODULE := ide_daemon.cfg
LOCAL_SRC_FILES := ../etc/ide_daemon.cfg

LOCAL_MODULE_CLASS := ETC

LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/ide_daemon.cfg
include $(BUILD_PREBUILT)

######################################
# add ide_daemon.ca to device
include $(CLEAR_VARS)

LOCAL_MODULE := ide_daemon_cacert.pem
LOCAL_SRC_FILES := ../etc/ide_daemon_cacert.pem

LOCAL_MODULE_CLASS := ETC

LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/ide_daemon_cacert.pem
include $(BUILD_PREBUILT)

######################################
# add ide_daemon client cert to device
include $(CLEAR_VARS)

LOCAL_MODULE := ide_daemon_client_cert.pem
LOCAL_SRC_FILES := ../etc/ide_daemon_client_cert.pem

LOCAL_MODULE_CLASS := ETC

LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/ide_daemon_client_cert.pem
include $(BUILD_PREBUILT)

######################################
# add ide_daemon client key to device
include $(CLEAR_VARS)

LOCAL_MODULE := ide_daemon_client_key.pem
LOCAL_SRC_FILES := ../etc/ide_daemon_client_key.pem

LOCAL_MODULE_CLASS := ETC

LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/ide_daemon_client_key.pem
include $(BUILD_PREBUILT)

######################################
# add ide_daemon server cert to device
include $(CLEAR_VARS)

LOCAL_MODULE := ide_daemon_server_cert.pem
LOCAL_SRC_FILES := ../etc/ide_daemon_server_cert.pem

LOCAL_MODULE_CLASS := ETC

LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/ide_daemon_server_cert.pem
include $(BUILD_PREBUILT)

######################################
# add ide_daemon server key to device
include $(CLEAR_VARS)

LOCAL_MODULE := ide_daemon_server_key.pem
LOCAL_SRC_FILES := ../etc/ide_daemon_server_key.pem

LOCAL_MODULE_CLASS := ETC

LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/ide_daemon_server_key.pem
include $(BUILD_PREBUILT)

######################################
# add ide_cmd.sh to device
include $(CLEAR_VARS)

LOCAL_MODULE := ide_cmd.sh
LOCAL_SRC_FILES := ../etc/ide_cmd.sh

LOCAL_MODULE_CLASS := ETC

LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/ide_cmd.sh
include $(BUILD_PREBUILT)

######################################
# add ide_daemon_monitor.sh
include $(CLEAR_VARS)

LOCAL_MODULE := ide_daemon_monitor.sh
LOCAL_SRC_FILES := ../etc/ide_daemon_monitor.sh

LOCAL_MODULE_CLASS := ETC

LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/ide_daemon_monitor.sh
include $(BUILD_PREBUILT)


######################################
# add setup_ide_daemon.sh for lhisi
include $(CLEAR_VARS)

LOCAL_MODULE := setup_ide_daemon.sh

ifeq ($(device_os),android)
LOCAL_SRC_FILES := ../etc/lhisi/android/setup_ide_daemon.sh
else
LOCAL_SRC_FILES := ../etc/lhisi/setup_ide_daemon.sh
endif

LOCAL_MODULE_CLASS := ETC

LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/setup_ide_daemon.sh
include $(BUILD_PREBUILT)

ifeq (${product}, mdc)
######################################
# add mdc_ide_daemon.service
include $(CLEAR_VARS)

LOCAL_MODULE := mdc_ide_daemon.service
LOCAL_SRC_FILES := ../etc/device/mdc_ide_daemon.service

LOCAL_MODULE_CLASS := ETC

LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/mdc_ide_daemon.service
include $(BUILD_PREBUILT)

######################################
# add mdc_ide_daemon_start.sh
include $(CLEAR_VARS)

LOCAL_MODULE := mdc_ide_daemon_start.sh
LOCAL_SRC_FILES := ../etc/device/mdc_ide_daemon_start.sh

LOCAL_MODULE_CLASS := ETC

LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/mdc_ide_daemon_start.sh
include $(BUILD_PREBUILT)

######################################
# add mdc_ide_daemon_device_install.sh
include $(CLEAR_VARS)

LOCAL_MODULE := mdc_ide_daemon_device_install.sh
LOCAL_SRC_FILES := ../etc/device/mdc_ide_daemon_device_install.sh

LOCAL_MODULE_CLASS := ETC

LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/mdc_ide_daemon_device_install.sh
include $(BUILD_PREBUILT)

######################################
# add ada.yaml
include $(CLEAR_VARS)

LOCAL_MODULE := ada.yaml
LOCAL_SRC_FILES := ../etc/device/ada.yaml

LOCAL_MODULE_CLASS := ETC

LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/ada.yaml
include $(BUILD_PREBUILT)
endif
