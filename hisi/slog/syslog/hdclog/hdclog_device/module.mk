LOCAL_PATH := $(call my-dir)

src_files := hdclog_device_init.c \
            ../../shared/log_level_parse.cpp \
            ../../shared/log_common_util.c

inc_files := abl/libc_sec/include \
            toolchain/ide/ide-daemon/hdc-common \
            ${LOCAL_PATH} \
            inc/driver \
            ${LOCAL_PATH}/../../shared \
            inc/toolchain \
            inc/mmpa

logdaemon_src_files := ../hdclog_host/log_recv_interface.c \
                       ../hdclog_host/log_recv.c \
                       ../hdclog_host/log_daemon.c \
                       ../../shared/msg_queue.c \
                       ../../shared/log_to_file.c \
                       ../../shared/cfg_file_parse.c \
                       ../../shared/log_sys_package.c \
                       ../../shared/print_log.c \
                       ../../shared/start_single_process.c \
                       ../../shared/log_queue.c \
                       ../../shared/log_zip.c \
                       ../../shared/log_level_parse.cpp \
                       ../../shared/log_common_util.c \
                       ../../slog/slog/src/log_monitor.c \
                       ../../slog/common/common.c

logdaemon_inc_files := abl/libc_sec/include \
                       toolchain/log/hdclog/hdclog_host \
                       ${LOCAL_PATH} \
                       inc/driver \
                       toolchain/ide/ide-daemon/hdc-common \
                       libkmc/include \
                       libkmc/src/sdp \
                       libkmc/src/common \
                       toolchain/log/shared \
                       inc/toolchain \
                       config/user_config \
                       toolchain/log/slog/slog/src \
                       toolchain/log/slog/common \
                       ${MDC_LINUX_SDK_DIR}/include

##########################################
# libhdclog_device.a for device
include $(CLEAR_VARS)
LOCAL_MODULE := libhdclog_device
ifeq (${product}, cloud)
  LOCAL_CFLAGS += -D_DEV_CLOUD_
endif

LOCAL_SRC_FILES := $(src_files)
LOCAL_C_INCLUDES := $(inc_files)
LOCAL_CFLAGS += -Werror -DIDE_DAEMON_DEVICE -DOS_TYPE_DEF=0 -fstack-protector
LOCAL_LDFLAGS := -lpthread -lc_sec
LOCAL_SHARED_LIBRARIES := libc_sec libmmpa
include $(BUILD_STATIC_LIBRARY)

##########################################
# libhdclog_device.a for host as debug
include $(CLEAR_VARS)
LOCAL_MODULE := libhdclog_device
ifeq (${product}, cloud)
  scene_cloud := -D_DEV_CLOUD_
endif

LOCAL_SRC_FILES := $(src_files)
LOCAL_C_INCLUDES := $(inc_files)
LOCAL_CFLAGS := -Werror ${scene_cloud}
LOCAL_LDFLAGS := -lpthread -lc_sec
LOCAL_SHARED_LIBRARIES := libc_sec libmmpa
include $(BUILD_HOST_STATIC_LIBRARY)


ifeq ($(product)_$(chip_id), mdc_hi1951)
#############################################################
include $(CLEAR_VARS)
LOCAL_MODULE := log-daemon
LOCAL_SRC_FILES := $(logdaemon_src_files)
LOCAL_C_INCLUDES := $(logdaemon_inc_files)

ifeq ($(GCC_VERSION_FLAG), 1)
LOCAL_CFLAGS += -fstack-protector-strong
else
LOCAL_CFLAGS += -fstack-protector-all
endif

LOCAL_CFLAGS += -Werror -DIDE_DAEMON_HOST -DOS_TYPE_DEF=0 -D_DEV_CLOUD_ -D_LOG_DAEMON_T_ -fPIE -DRECV_BY_DRV -DHARDWARE_ZIP -DGETCLOCK_VIRTUAL -DIAM -DSORTED_LOG -DRC_MODE
LOCAL_SHARED_LIBRARIES += libc_sec libascend_hal libwd
LOCAL_LDFLAGS += -lpthread -lc_sec -Wl,-z,relro,-z,now -Wl,-z,noexecstack -pie \
                 -L${MDC_LINUX_SDK_DIR}/lib64 \
                 -L${MDC_LINUX_SDK_DIR}/lib64/mdc/base-plat \
                 -liam -leasy_comm -lxshmem

### Add for bbox
LOCAL_STATIC_LIBRARIES += libbboxhagent
LOCAL_SHARED_LIBRARIES += libslog libmmpa
###
include $(BUILD_EXECUTABLE)

# add log-daemon.yaml to device for mdc
include $(CLEAR_VARS)
LOCAL_MODULE := log-daemon.yaml
LOCAL_SRC_FILES := ../../slog/slog/etc/1951_mdc/log-daemon.yaml
LOCAL_MODULE_CLASS := ETC
LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/log-daemon.yaml
include $(BUILD_PREBUILT)

#############################################################
endif

ifeq ($(product)_$(chip_id), mdc_hi1951pg2)
#############################################################
include $(CLEAR_VARS)
LOCAL_MODULE := log-daemon
LOCAL_SRC_FILES := $(logdaemon_src_files)
LOCAL_C_INCLUDES := $(logdaemon_inc_files)

ifeq ($(GCC_VERSION_FLAG), 1)
LOCAL_CFLAGS += -fstack-protector-strong
else
LOCAL_CFLAGS += -fstack-protector-all
endif

LOCAL_CFLAGS += -Werror -DIDE_DAEMON_HOST -DOS_TYPE_DEF=0 -D_DEV_CLOUD_ -D_LOG_DAEMON_T_ -fPIE -DRECV_BY_DRV -DHARDWARE_ZIP -DRC_MODE
LOCAL_SHARED_LIBRARIES += libc_sec libascend_hal libwd
LOCAL_LDFLAGS += -lpthread -lc_sec -Wl,-z,relro,-z,now -Wl,-z,noexecstack -pie

### Add for bbox
LOCAL_STATIC_LIBRARIES += libbboxhagent
LOCAL_SHARED_LIBRARIES += libslog libmmpa
###
include $(BUILD_EXECUTABLE)

# add log-daemon.yaml to device for mdc pg2
include $(CLEAR_VARS)
LOCAL_MODULE := log-daemon.yaml
LOCAL_SRC_FILES := ../../slog/slog/etc/1951_mdc/log-daemon.yaml
LOCAL_MODULE_CLASS := ETC
LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/log-daemon.yaml
include $(BUILD_PREBUILT)

#############################################################
endif

ifeq ($(product)_$(rc), mini_mini)
#############################################################
include $(CLEAR_VARS)
LOCAL_MODULE := log-daemon
LOCAL_SRC_FILES := ../hdclog_host/log_recv_interface.c \
                   ../hdclog_host/log_recv.c \
                   ../hdclog_host/log_daemon.c \
                   ../../shared/msg_queue.c \
                   ../../shared/log_to_file.c \
                   ../../shared/cfg_file_parse.c \
                   ../../shared/log_sys_package.c \
                   ../../shared/print_log.c \
                   ../../shared/start_single_process.c \
                   ../../shared/log_queue.c \
                   ../../shared/log_level_parse.cpp \
                   ../../shared/log_common_util.c \
                   ../../shared/log_zip.c \
                   ../../slog/slog/src/log_monitor.c \
                   ../../slog/common/common.c

LOCAL_C_INCLUDES := abl/libc_sec/include \
                    toolchain/log/hdclog/hdclog_host \
                    ${LOCAL_PATH} \
                    inc/driver \
                    toolchain/ide/ide-daemon/hdc-common \
                    libkmc/include \
                    libkmc/src/sdp \
                    libkmc/src/common \
                    toolchain/log/shared \
                    inc/toolchain \
                    third_party/zlib/zlib-1.2.11 \
                    config/user_config \
                    toolchain/log/slog/slog/src \
                    toolchain/log/slog/common

ifeq ($(GCC_VERSION_FLAG), 1)
LOCAL_CFLAGS += -fstack-protector-strong
else
LOCAL_CFLAGS += -fstack-protector-all
endif

LOCAL_CFLAGS += -Werror -DIDE_DAEMON_HOST -DOS_TYPE_DEF=0 -D_LOG_DAEMON_T_ -fPIE -DRECV_BY_DRV -DRC_MODE
LOCAL_LDFLAGS := -lpthread -lc_sec -Wl,-z,relro,-z,now -Wl,-z,noexecstack -pie
LOCAL_SHARED_LIBRARIES += libc_sec libascend_hal
### Add for bbox
LOCAL_STATIC_LIBRARIES += libbboxhagent libzlib
LOCAL_SHARED_LIBRARIES += libslog libmmpa
###
include $(BUILD_EXECUTABLE)

#############################################################
endif


ifeq ($(product), cloud)
#############################################################
include $(CLEAR_VARS)
LOCAL_MODULE := log-daemon
LOCAL_SRC_FILES := ../hdclog_host/log_recv_interface.c \
                   ../hdclog_host/log_recv.c \
                   ../hdclog_host/log_daemon.c \
                   ../../shared/msg_queue.c \
                   ../../shared/log_to_file.c \
                   ../../shared/cfg_file_parse.c \
                   ../../shared/log_sys_package.c \
                   ../../shared/print_log.c \
                   ../../shared/start_single_process.c \
                   ../../shared/log_queue.c \
                   ../../shared/log_level_parse.cpp \
                   ../../shared/log_common_util.c \
                   ../../shared/log_zip.c \
                   ../../slog/slog/src/log_monitor.c \
                   ../../slog/common/common.c

LOCAL_C_INCLUDES := abl/libc_sec/include \
                    toolchain/log/hdclog/hdclog_host \
                    ${LOCAL_PATH} \
                    inc/driver \
                    toolchain/ide/ide-daemon/hdc-common \
                    libkmc/include \
                    libkmc/src/sdp \
                    libkmc/src/common \
                    toolchain/log/shared \
                    inc/toolchain \
                    third_party/zlib/zlib-1.2.11 \
                    config/user_config \
                    toolchain/log/slog/slog/src \
                    toolchain/log/slog/common

ifeq ($(GCC_VERSION_FLAG), 1)
LOCAL_CFLAGS += -fstack-protector-strong
else
LOCAL_CFLAGS += -fstack-protector-all
endif

LOCAL_CFLAGS += -Werror -DIDE_DAEMON_HOST -DOS_TYPE_DEF=0 -D_LOG_DAEMON_T_ -fPIE -DRECV_BY_DRV -D_LOG_MONITOR_ -D_NONE_ZIP_ -D_SLOG_DEVICE_ -DEP_DEVICE_MODE
LOCAL_LDFLAGS := -lpthread -lc_sec -Wl,-z,relro,-z,now -Wl,-z,noexecstack -pie
LOCAL_SHARED_LIBRARIES += libc_sec libascend_hal libascend_monitor libslog libmmpa
LOCAL_STATIC_LIBRARIES += libbboxhagent
include $(BUILD_EXECUTABLE)

######################################
# add log_daemon_monitor.sh to device
include $(CLEAR_VARS)
LOCAL_MODULE := log_daemon_monitor.sh
LOCAL_SRC_FILES := ../../scripts/log_daemon_monitor.sh
LOCAL_MODULE_CLASS := ETC
LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/log_daemon_monitor.sh
include $(BUILD_PREBUILT)

#############################################################
endif

ifeq ($(product), mini)
ifneq ($(rc), mini)
#############################################################
include $(CLEAR_VARS)
LOCAL_MODULE := log-daemon
LOCAL_SRC_FILES := ../hdclog_host/log_recv_interface.c \
                   ../hdclog_host/log_recv.c \
                   ../hdclog_host/log_daemon.c \
                   ../../shared/msg_queue.c \
                   ../../shared/log_to_file.c \
                   ../../shared/cfg_file_parse.c \
                   ../../shared/log_sys_package.c \
                   ../../shared/print_log.c \
                   ../../shared/start_single_process.c \
                   ../../shared/log_queue.c \
                   ../../shared/log_level_parse.cpp \
                   ../../shared/log_common_util.c \
                   ../../shared/log_zip.c \
                   ../../slog/slog/src/log_monitor.c \
                   ../../slog/common/common.c

LOCAL_C_INCLUDES := abl/libc_sec/include \
                    toolchain/log/hdclog/hdclog_host \
                    ${LOCAL_PATH} \
                    inc/driver \
                    toolchain/ide/ide-daemon/hdc-common \
                    libkmc/include \
                    libkmc/src/sdp \
                    libkmc/src/common \
                    toolchain/log/shared \
                    inc/toolchain \
                    third_party/zlib/zlib-1.2.11 \
                    config/user_config \
                    toolchain/log/slog/slog/src \
                    toolchain/log/slog/common

ifeq ($(GCC_VERSION_FLAG), 1)
LOCAL_CFLAGS += -fstack-protector-strong
else
LOCAL_CFLAGS += -fstack-protector-all
endif

LOCAL_CFLAGS += -Werror -DIDE_DAEMON_HOST -DOS_TYPE_DEF=0 -D_LOG_DAEMON_T_ -fPIE -DRECV_BY_DRV -D_LOG_MONITOR_ -D_NONE_ZIP_ -D_SLOG_DEVICE_ -DEP_DEVICE_MODE
LOCAL_LDFLAGS := -lpthread -lc_sec -Wl,-z,relro,-z,now -Wl,-z,noexecstack -pie
LOCAL_SHARED_LIBRARIES += libc_sec libascend_hal libascend_monitor libslog libmmpa
LOCAL_STATIC_LIBRARIES += libbboxhagent
include $(BUILD_EXECUTABLE)

######################################
# add log_daemon_monitor.sh to device
include $(CLEAR_VARS)
LOCAL_MODULE := log_daemon_monitor.sh
LOCAL_SRC_FILES := ../../scripts/log_daemon_monitor.sh
LOCAL_MODULE_CLASS := ETC
LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/log_daemon_monitor.sh
include $(BUILD_PREBUILT)

#############################################################
endif
endif
