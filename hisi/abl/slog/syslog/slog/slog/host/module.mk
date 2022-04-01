LOCAL_PATH := $(call my-dir)

slogd_src_path := ../src
slogd_src_files := ${slogd_src_path}/syslogd.c \
                   ${slogd_src_path}/syslogd_common.c \
                   ${slogd_src_path}/set_loglevel.c \
                   ../../../shared/msg_queue.c \
                   ../../../shared/share_mem.c \
                   ../../../shared/log_to_file.c \
                   ${slogd_src_path}/slogd_main.c \
                   ${slogd_src_path}/log_monitor.c \
                   ../../common/common.c \
                   ../../../shared/start_single_process.c \
                   ../../../shared/cfg_file_parse.c \
                   ../../../shared/log_zip.c \
                   ../../../shared/print_log.c \
                   ../../../shared/log_level_parse.cpp \
                   ../../../shared/log_common_util.c \
                   ../../../shared/log_sys_package.c

slogd_inc_files :=  ${LOCAL_PATH}/../../../../../abl/libc_sec/include \
                    ${LOCAL_PATH}/../../slog \
                    ${LOCAL_PATH}/../../common \
                    ${LOCAL_PATH}/../../../shared \
                    third_party/zlib/zlib-1.2.11 \
                    inc/driver \
                    inc/toolchain

####################################
# build slogd host
ifneq (${product}, mdc)
ifneq (${product}, mini)
ifneq (${product}, cloud)
include $(CLEAR_VARS)

LOCAL_MODULE := slogd
LOCAL_SRC_FILES := $(slogd_src_files)
LOCAL_C_INCLUDES := $(slogd_inc_files)

ifeq ($(product), cloud)
    LOCAL_CFLAGS += -D_HOST_CLOUD -D_LOG_MONITOR_ -D_NONE_ZIP_ -DLARGE_LOGFILE_VOLUME
    LOCAL_SHARED_LIBRARIES += libascend_monitor
else ifeq ($(chip_id), hi1951)
    LOCAL_CFLAGS += -D_NONE_ZIP_
    ifeq ($(product),mini)
        LOCAL_CFLAGS += -DLARGE_LOGFILE_VOLUME -D_LOG_MONITOR_
        LOCAL_SHARED_LIBRARIES +=  libascend_monitor
    endif
else ifeq ($(product), mdc)
    LOCAL_CFLAGS += -D_NONE_ZIP_ -DLARGE_LOGFILE_NUM
else ifeq ($(product), mini)
    LOCAL_STATIC_LIBRARIES := libzlib
    ifneq ($(rc), mini)
        LOCAL_CFLAGS += -D_LOG_MONITOR_ -DLARGE_LOGFILE_VOLUME
        LOCAL_SHARED_LIBRARIES += libascend_monitor
    endif
else
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

LOCAL_CFLAGS += -Werror -DHOST_WRITE_FILE=1 -fPIE -pie -D_FORTIFY_SOURCE=2 -O2 -DOS_TYPE_DEF=0
LOCAL_LDFLAGS := -lpthread -lc_sec -Wl,-z,relro -Wl,-z,noexecstack -fPIE -pie -Wl,-z,now
LOCAL_SHARED_LIBRARIES += libc_sec
include $(BUILD_HOST_EXECUTABLE)

endif
endif
endif
####################################
# build slogd docker
ifeq (${product},cloud)
include $(CLEAR_VARS)

LOCAL_MODULE := docker/slogd

LOCAL_SRC_FILES := $(slogd_src_files)
LOCAL_C_INCLUDES := $(slogd_inc_files)

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

LOCAL_CFLAGS += -Werror -DHOST_WRITE_FILE=1 -fPIE -pie -D_FORTIFY_SOURCE=2 -O2 -DSLOG_DEBUG -DOS_TYPE_DEF=0 -D_HOST_CLOUD -D_CLOUD_DOCKER -DLARGE_LOGFILE_VOLUME
LOCAL_LDFLAGS := -lpthread -lc_sec -Wl,-z,relro -Wl,-z,noexecstack -fPIE -pie -Wl,-z,now
LOCAL_SHARED_LIBRARIES += libc_sec
LOCAL_STATIC_LIBRARIES := libzlib
include $(BUILD_HOST_EXECUTABLE)

endif

######################################
# add slog.conf to host
include $(CLEAR_VARS)

LOCAL_MODULE := slog.conf

ifeq (${product},cloud)
  LOCAL_SRC_FILES := ../etc/cloud/slog.conf
else ifeq (${product},mdc)
  LOCAL_SRC_FILES := ../etc/1910_mdc/slog.conf
else ifeq ($(product)_$(chip_id), mini_hi1951)
  LOCAL_SRC_FILES := ../etc/1951_dc/slog.conf
else ifeq (${product},lhisi)
  LOCAL_SRC_FILES := ../etc/lhisi/slog.conf
else ifeq (${product}_${rc}, mini_mini)
  LOCAL_SRC_FILES := ../etc/mini_rc/slog.conf
else
  LOCAL_SRC_FILES := ../etc/mini/slog.conf
endif

LOCAL_MODULE_CLASS := ETC
LOCAL_INSTALLED_PATH := $(HOST_OUT_ROOT)/slog.conf
include $(BUILD_HOST_PREBUILT)

######################################
# add slog.conf to host
include $(CLEAR_VARS)

LOCAL_MODULE := win_slog_conf
LOCAL_SRC_FILES := ../winconf/slog.conf

LOCAL_MODULE_CLASS := ETC

LOCAL_INSTALLED_PATH := $(HOST_OUT_ROOT)/winconf/slog.conf
include $(BUILD_HOST_PREBUILT)

######################################
# add slogd_daemon_monitor.sh to host
include $(CLEAR_VARS)

LOCAL_MODULE := slogd_daemon_monitor.sh
LOCAL_SRC_FILES := ../../../scripts/slogd_daemon_monitor.sh

LOCAL_MODULE_CLASS := ETC

LOCAL_INSTALLED_PATH := $(HOST_OUT_ROOT)/slogd_daemon_monitor.sh
include $(BUILD_HOST_PREBUILT)

######################################
# add log_daemon_monitor.sh to host
include $(CLEAR_VARS)

LOCAL_MODULE := log_daemon_monitor.sh
LOCAL_SRC_FILES := ../../../scripts/log_daemon_monitor.sh

LOCAL_MODULE_CLASS := ETC

LOCAL_INSTALLED_PATH := $(HOST_OUT_ROOT)/log_daemon_monitor.sh
include $(BUILD_HOST_PREBUILT)

ifeq (${product}, mdc)
######################################
# add mdc_slog.service to host for mdc
include $(CLEAR_VARS)

LOCAL_MODULE := mdc_slog.service
LOCAL_SRC_FILES := ../../../mdc/host/mdc_slog.service

LOCAL_MODULE_CLASS := ETC

LOCAL_INSTALLED_PATH := $(HOST_OUT_ROOT)/mdc_slog.service
include $(BUILD_HOST_PREBUILT)

######################################
# add mdc_slog_start.sh to host for mdc
include $(CLEAR_VARS)

LOCAL_MODULE := mdc_slog_start.sh
LOCAL_SRC_FILES := ../../../mdc/host/mdc_slog_start.sh

LOCAL_MODULE_CLASS := ETC

LOCAL_INSTALLED_PATH := $(HOST_OUT_ROOT)/mdc_slog_start.sh
include $(BUILD_HOST_PREBUILT)

######################################
# add mdc_slog_host_install.sh to host for mdc
include $(CLEAR_VARS)

LOCAL_MODULE := mdc_slog_host_install.sh
LOCAL_SRC_FILES := ../../../mdc/host/mdc_slog_host_install.sh

LOCAL_MODULE_CLASS := ETC

LOCAL_INSTALLED_PATH := $(HOST_OUT_ROOT)/mdc_slog_host_install.sh
include $(BUILD_HOST_PREBUILT)
endif
