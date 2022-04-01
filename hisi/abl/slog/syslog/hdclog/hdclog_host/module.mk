
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libhdclog_host
LOCAL_SRC_FILES := hdclog_host_init.c \
                   hdclog_file.c \
                   ../../shared/msg_queue.c \
                   ../../shared/print_log.c \
                   ../../shared/log_level_parse.cpp \
                   ../../shared/log_common_util.c

LOCAL_C_INCLUDES := abl/libc_sec/include \
                    toolchain/ide/ide-daemon/hdc-common \
                    libkmc/include \
                    libkmc/src/sdp \
                    libkmc/src/common \
                    ${LOCAL_PATH} \
                    inc/driver \
                    ${LOCAL_PATH}/../../shared \
                    inc/toolchain \
                    inc/mmpa

ifeq ($(product), cloud)
  LOCAL_CFLAGS += -D_HOST_CLOUD -D_LOG_MONITOR_
  LOCAL_SHARED_LIBRARIES += libascend_monitor
else
  LOCAL_SRC_FILES += ../../shared/log_zip.c
  LOCAL_C_INCLUDES += third_party/zlib/zlib-1.2.11
endif

LOCAL_CFLAGS += -Werror -DOS_TYPE_DEF=0 -fstack-protector
LOCAL_LDFLAGS := -lpthread -lc_sec
LOCAL_SHARED_LIBRARIES += libc_sec
#LOCAL_UNINSTALLABLE_MODULE :=false
include $(BUILD_HOST_STATIC_LIBRARY)
############################################################
include $(CLEAR_VARS)
LOCAL_MODULE := libhdclog_host
LOCAL_SRC_FILES := hdclog_host_init.c \
                   hdclog_file.c \
                   ../../shared/cfg_file_parse.c \
                   ../../shared/msg_queue.c \
                   ../../shared/print_log.c \
                   ../../shared/share_mem.c \
                   ../../shared/log_level_parse.cpp \
                   ../../shared/log_common_util.c \
                   ../../shared/log_sys_package.c

LOCAL_C_INCLUDES := abl/libc_sec/include \
                    toolchain/ide/ide-daemon/hdc-common \
                    libkmc/include \
                    libkmc/src/sdp \
                    libkmc/src/common \
                    ${LOCAL_PATH} \
                    inc/driver \
                    ${LOCAL_PATH}/../../shared \
                    inc/toolchain \
                    inc/mmpa

ifeq ($(product)_$(rc), mini_mini)
    LOCAL_CFLAGS += -D_MINI_RC -DLHISI_SOCKET
endif

# _DEV_CLOUD_ to drop device threads;
ifeq ($(product), lhisi)
  LOCAL_CFLAGS += -DLHISI_SOCKET -D_DEV_CLOUD_ -D_NONE_ZIP_
else ifeq ($(product), mdc)
  ifeq ($(chip_id), hi1951)
    LOCAL_CFLAGS += -DLHISI_SOCKET -D_DEV_CLOUD_ -D_NONE_ZIP_
  else ifeq ($(chip_id), hi1951pg2)
    LOCAL_CFLAGS += -DLHISI_SOCKET -D_DEV_CLOUD_ -D_NONE_ZIP_
  endif
else
  LOCAL_SRC_FILES += ../../shared/log_zip.c
  LOCAL_C_INCLUDES += third_party/zlib/zlib-1.2.11
endif

LOCAL_CFLAGS += -Werror -DOS_TYPE_DEF=0 -fstack-protector
LOCAL_LDFLAGS := -lpthread -lc_sec
LOCAL_SHARED_LIBRARIES := libc_sec
include $(BUILD_STATIC_LIBRARY)
#############################################################
#log-daemon(host) not support for 1951_mdc, minirc or lhisi
ifneq ($(product)_$(chip_id), mdc_hi1951pg2)
ifneq ($(product)_$(chip_id), mdc_hi1951)
ifneq ($(product)_$(rc), mini_mini)
ifneq ($(product), lhisi)

include $(CLEAR_VARS)
LOCAL_MODULE := log-daemon-host
LOCAL_SRC_FILES := log_recv_interface.c \
                   log_recv.c \
                   log_daemon.c \
                   ../../shared/msg_queue.c \
                   ../../shared/log_to_file.c \
                   ../../shared/start_single_process.c \
                   ../../shared/log_queue.c \
                   ../../shared/log_level_parse.cpp \
                   ../../shared/log_common_util.c \
                   ../../slog/slog/src/log_monitor.c \
                   ../../slog/common/common.c \
                   ../../shared/log_sys_package.c \
                   ../../shared/print_log.c \
                   ../../shared/cfg_file_parse.c

LOCAL_C_INCLUDES := abl/libc_sec/include \
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
                    toolchain/log/slog/common

ifeq (${product}, cloud)
  LOCAL_CFLAGS += -D_LOG_MONITOR_ -D_HOST_CLOUD -D_NONE_ZIP_ -DLARGE_LOGFILE_VOLUME
  LOCAL_SHARED_LIBRARIES += libascend_monitor
else ifeq ($(chip_id), hi1951)
  LOCAL_CFLAGS += -D_NONE_ZIP_
  ifeq (${product}, mini)
    LOCAL_CFLAGS += -DLARGE_LOGFILE_VOLUME -D_LOG_MONITOR_
    LOCAL_SHARED_LIBRARIES += libascend_monitor
  endif
else ifeq ($(product), mdc)
  LOCAL_CFLAGS += -D_NONE_ZIP_ -DLARGE_LOGFILE_NUM
else ifeq ($(product), mini)
  ifneq ($(rc), mini)
    LOCAL_CFLAGS += -D_LOG_MONITOR_ -DLARGE_LOGFILE_VOLUME
    LOCAL_SHARED_LIBRARIES += libascend_monitor
  endif
  LOCAL_SRC_FILES += ../../shared/log_zip.c
  LOCAL_C_INCLUDES += third_party/zlib/zlib-1.2.11
  LOCAL_STATIC_LIBRARIES := libzlib
else
  LOCAL_SRC_FILES += ../../shared/log_zip.c
  LOCAL_C_INCLUDES += third_party/zlib/zlib-1.2.11
  LOCAL_STATIC_LIBRARIES := libzlib
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

LOCAL_CFLAGS += -Werror -DIDE_DAEMON_HOST -DOS_TYPE_DEF=0 -fPIE -D_LOG_DAEMON_T_ -DLOG_COREDUMP
LOCAL_LDFLAGS := -lpthread -lc_sec -Wl,-z,relro,-z,now -Wl,-z,noexecstack -pie
LOCAL_SHARED_LIBRARIES += libc_sec libascend_hal
LOCAL_STATIC_LIBRARIES += libKMC libSDP
### Add for bbox
LOCAL_C_INCLUDES += toolchain/bbox/bboxhagent/bbox_excep_mgr/basic_ability/register \
                    toolchain/bbox/bboxhagent/bbox_excep_mgr/include \
                    toolchain/ide/ide-daemon \
                    toolchain/ide/ide-daemon/external \

LOCAL_SHARED_LIBRARIES += libslog libmmpa
LOCAL_STATIC_LIBRARIES += libadcore
###
#LOCAL_UNINSTALLABLE_MODULE :=false
include $(BUILD_HOST_EXECUTABLE)

endif
endif
endif
endif
