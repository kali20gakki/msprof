LOCAL_PATH := $(call my-dir)

slogd_src_path := ../src
slogd_src_files := ${slogd_src_path}/syslogd.c \
                   ${slogd_src_path}/syslogd_common.c \
                   ${slogd_src_path}/operate_loglevel.c \
                   ../../../shared/msg_queue.c \
                   ../../../shared/share_mem.c \
                   ../../../shared/print_log.c \
                   ${slogd_src_path}/slogd_main.c \
                   ${slogd_src_path}/log_monitor.c \
                   ${slogd_src_path}/log_dump.c \
                   ${slogd_src_path}/app_log_watch.c \
                   ../../common/common.c \
                   ../../../shared/log_zip.c \
                   ../../../shared/start_single_process.c \
                   ../../../shared/cfg_file_parse.c \
                   ../../../shared/log_level_parse.cpp \
                   ../../../shared/log_common_util.c \
                   ../../../shared/log_sys_package.c \
                   ../../../shared/log_session_manage.c

slogd_inc_files :=  ${LOCAL_PATH}/../../../../../abl/libc_sec/include \
                    ${LOCAL_PATH}/../../slog \
                    ${LOCAL_PATH}/../../common \
                    ${LOCAL_PATH}/../../../shared \
                    third_party/zlib/zlib-1.2.11 \
                    ${LOCAL_PATH}/../../../../../kernel/vendor/nve/inlcude/linux/mtd \
                    inc/driver \
                    inc/toolchain/stackcore \
                    inc/toolchain \
                    toolchain/ide/ide-daemon/external


iam_slogd_src_files := ${slogd_src_path}/syslogd_iam.c \
                       ${slogd_src_path}/syslogd_common.c \
                       ${slogd_src_path}/operate_loglevel.c \
                       ../../../shared/msg_queue.c \
                       ../../../shared/share_mem.c \
                       ../../../shared/print_log.c \
                       ${slogd_src_path}/slogd_main.c \
                       ${slogd_src_path}/log_monitor.c \
                       ${slogd_src_path}/log_dump.c \
                       ${slogd_src_path}/app_log_watch.c \
                       ../../common/common.c \
                       ../../../shared/log_zip.c \
                       ../../../shared/start_single_process.c \
                       ../../../shared/cfg_file_parse.c \
                       ../../../shared/log_level_parse.cpp \
                       ../../../shared/log_common_util.c \
                       ../../../shared/log_sys_package.c \
                       ../../../shared/log_to_file.c

iam_slogd_inc_files :=  ${LOCAL_PATH}/../../../../../abl/libc_sec/include \
                        ${LOCAL_PATH}/../../slog \
                        ${LOCAL_PATH}/../../common \
                        ${LOCAL_PATH}/../../include \
                        ${LOCAL_PATH}/../../../shared \
                        third_party/zlib/zlib-1.2.11 \
                        ${LOCAL_PATH}/../../../../../kernel/vendor/nve/inlcude/linux/mtd \
                        ${MDC_LINUX_SDK_DIR}/include \
                        inc/driver \
                        inc/toolchain \
                        toolchain/ide/ide-daemon/external

####################################
# build slogd device
include $(CLEAR_VARS)
LOCAL_MODULE := slogd

ifeq ($(product), mini)
    ifneq ($(chip_id), hi1951)
        LOCAL_CFLAGS += -DSTACKCORE
        LOCAL_SHARED_LIBRARIES += libstackcore
        ifneq ($(rc), mini)
            LOCAL_CFLAGS += -D_LOG_MONITOR_ -DEP_DEVICE_MODE
            LOCAL_SHARED_LIBRARIES += libascend_monitor libadump
        endif
    endif
endif

# in lhisi-npuf10 & 1951mdc, slogd will write log file
ifeq ($(product)_$(rc), mini_mini)
    LOCAL_STATIC_LIBRARIES := libzlib
    LOCAL_SHARED_LIBRARIES += libascend_hal
    LOCAL_CFLAGS += -DHOST_WRITE_FILE -D_SLOG_DEVICE_ -DAPP_LOG_WATCH -DRC_MODE
    slogd_src_files += ../../../shared/log_to_file.c
else ifeq ($(product)_$(chip_id), lhisi_npuf10)
    LOCAL_STATIC_LIBRARIES := libzlib
    LOCAL_CFLAGS += -DHOST_WRITE_FILE -DLHISI_ -DAPP_LOG_WATCH -DRC_HISI_MODE
    slogd_src_files += ../../../shared/log_to_file.c
else ifeq ($(product)_$(chip_id), mdc_hi1951)
    LOCAL_CFLAGS += -DHOST_WRITE_FILE -D_SLOG_DEVICE_ -DHARDWARE_ZIP -DGETCLOCK_VIRTUAL -DSORTED_LOG -DAPP_LOG_WATCH -DRC_MODE
    LOCAL_STATIC_LIBRARIES := libzlib
    LOCAL_SHARED_LIBRARIES += libascend_hal libwd
else ifeq ($(product)_$(chip_id), mdc_hi1951pg2)
    LOCAL_CFLAGS += -DHOST_WRITE_FILE -D_SLOG_DEVICE_ -DHARDWARE_ZIP -DRC_MODE
    LOCAL_STATIC_LIBRARIES := libzlib
    LOCAL_SHARED_LIBRARIES += libascend_hal libwd
    slogd_src_files += ../../../shared/log_to_file.c
else ifeq ($(product)_$(chip_id), mini_hi1951)
    LOCAL_STATIC_LIBRARIES += liblog_server libzlib
    LOCAL_SHARED_LIBRARIES += libascend_hal libadump libascend_monitor libslog libmmpa
    LOCAL_CFLAGS += -D_SLOG_DEVICE_ -DLOG_COREDUMP -D_LOG_MONITOR_ -DHOST_WRITE_FILE -DAPP_LOG_REPORT -DEP_DEVICE_MODE
    slogd_src_files += ../../../shared/log_to_file.c \
                        ../../../external/log_drv.c \
                        ../../../external/api/driver_api.c
    slogd_inc_files += ${LOCAL_PATH}/../../../external
else ifeq (${product}, cloud)
    LOCAL_CFLAGS += -D_SLOG_DEVICE_ -DLOG_COREDUMP -DHOST_WRITE_FILE -D_DEV_CLOUD_ -D_LOG_MONITOR_ -D_NONE_ZIP_ -DSTACKCORE -DAPP_LOG_REPORT -DEP_DEVICE_MODE
    LOCAL_STATIC_LIBRARIES += liblog_server
    LOCAL_SHARED_LIBRARIES += libascend_monitor libascend_hal libadump libstackcore libslog libmmpa
    slogd_src_files += ../../../shared/log_to_file.c \
                        ../../../external/log_drv.c \
                        ../../../external/api/driver_api.c
    slogd_inc_files += ${LOCAL_PATH}/../../../external
else
    LOCAL_STATIC_LIBRARIES := libzlib liblog_server
    LOCAL_SHARED_LIBRARIES += libascend_hal libslog libmmpa
    LOCAL_CFLAGS += -D_SLOG_DEVICE_ -DLOG_COREDUMP -DHOST_WRITE_FILE -DAPP_LOG_REPORT
    slogd_src_files += ../../../shared/log_to_file.c \
                        ../../../external/log_drv.c \
                        ../../../external/api/driver_api.c
    slogd_inc_files += ${LOCAL_PATH}/../../../external
endif

LOCAL_SHARED_LIBRARIES += libc_sec
ifeq ($(product)_$(chip_id), mdc_hi1951)
LOCAL_SRC_FILES := $(iam_slogd_src_files)
LOCAL_C_INCLUDES := $(iam_slogd_inc_files)
LOCAL_LDFLAGS += -L${MDC_LINUX_SDK_DIR}/lib64 \
                 -L${MDC_LINUX_SDK_DIR}/lib64/mdc/base-plat \
                 -liam -leasy_comm -lxshmem
else
LOCAL_SRC_FILES := $(slogd_src_files)
LOCAL_C_INCLUDES := $(slogd_inc_files)
endif

ifeq ($(device_os),android)
    LOCAL_CFLAGS += -fstack-protector-strong -fPIE -D_FORTIFY_SOURCE=2 -O2 -DOS_TYPE_DEF=0
    LOCAL_LDFLAGS += -ldl -lc_sec -Wl,-z,relro -Wl,-z,noexecstack -fPIE -Wl,-z,now
else
    LOCAL_CFLAGS += -Werror -fstack-protector-strong -fPIE -pie -D_FORTIFY_SOURCE=2 -O2 -DOS_TYPE_DEF=0
    LOCAL_LDFLAGS += -lpthread -ldl -lc_sec -Wl,-z,relro -Wl,-z,noexecstack -fPIE -pie -Wl,-z,now
endif

include $(BUILD_EXECUTABLE)

######################################
# add slog.conf to device
include $(CLEAR_VARS)
LOCAL_MODULE := slog.conf

ifeq (${product},cloud)
  LOCAL_SRC_FILES := ../etc/cloud/slog.conf
else ifeq (${product}, mdc)
  ifeq ($(chip_id), hi1951)
    LOCAL_SRC_FILES := ../etc/1951_mdc/slog.conf
  else ifeq ($(chip_id), hi1951pg2)
    LOCAL_SRC_FILES := ../etc/1951_mdc/slog.conf
  else
    LOCAL_SRC_FILES := ../etc/1910_mdc/slog.conf
  endif
else ifeq ($(product)_$(chip_id), mini_hi1951)
  LOCAL_SRC_FILES := ../etc/1951_dc/slog.conf
else ifeq (${product},lhisi)
  LOCAL_SRC_FILES := ../etc/lhisi/slog.conf
else ifeq ($(product)_$(rc), mini_mini)
  LOCAL_SRC_FILES := ../etc/mini_rc/slog.conf
else
  LOCAL_SRC_FILES := ../etc/mini/slog.conf
endif

LOCAL_MODULE_CLASS := ETC
LOCAL_CFLAGS += -Werror -DOS_TYPE_DEF=0
LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/slog.conf
include $(BUILD_PREBUILT)

######################################
# add slogd_daemon_monitor.sh to device
include $(CLEAR_VARS)
LOCAL_MODULE := slogd_daemon_monitor.sh
LOCAL_SRC_FILES := ../../../scripts/slogd_daemon_monitor.sh
LOCAL_MODULE_CLASS := ETC
LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/slogd_daemon_monitor.sh
include $(BUILD_PREBUILT)

######################################
include $(CLEAR_VARS)
LOCAL_MODULE := slogd_dft
scene_minirc := -DHOST_WRITE_FILE=1
ifeq (${product},cloud)
  scene_cloud := -D_DEV_CLOUD_
  LOCAL_SHARED_LIBRARIES := libc_sec libascend_hal
else
  LOCAL_SHARED_LIBRARIES := libc_sec libascend_hal
endif
LOCAL_SRC_FILES := $(slogd_src_files)
LOCAL_C_INCLUDES := $(slogd_inc_files)
LOCAL_CFLAGS := -Werror -fstack-protector-strong -fPIE -pie -D_FORTIFY_SOURCE=2 -O2 ${scene_minirc} -DOS_TYPE_DEF=0 ${scene_cloud} -D_SLOG_DEVICE_
LOCAL_LDFLAGS := -lpthread -lc_sec -Wl,-z,relro -Wl,-z,noexecstack -fPIE -pie -Wl,-z,now
LOCAL_STATIC_LIBRARIES := libzlib
include $(BUILD_EXECUTABLE)

ifeq (${product}, mdc)
######################################
# add mdc_slog.service to device for mdc
include $(CLEAR_VARS)
LOCAL_MODULE := mdc_slog.service
LOCAL_SRC_FILES := ../../../mdc/device/mdc_slog.service
LOCAL_MODULE_CLASS := ETC
LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/mdc_slog.service
include $(BUILD_PREBUILT)

######################################
# add mdc_slog_start.sh to device for mdc
include $(CLEAR_VARS)
LOCAL_MODULE := mdc_slog_start.sh
LOCAL_SRC_FILES := ../../../mdc/device/mdc_slog_start.sh
LOCAL_MODULE_CLASS := ETC
LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/mdc_slog_start.sh
include $(BUILD_PREBUILT)

######################################
# add mdc_slog_device_install.sh to device for mdc
include $(CLEAR_VARS)
LOCAL_MODULE := mdc_slog_device_install.sh
LOCAL_SRC_FILES := ../../../mdc/device/mdc_slog_device_install.sh
LOCAL_MODULE_CLASS := ETC
LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/mdc_slog_device_install.sh
include $(BUILD_PREBUILT)

######################################
# add unzip_tool.py to device for mdc
include $(CLEAR_VARS)
LOCAL_MODULE := unzip_tool.py
LOCAL_SRC_FILES := ../../../scripts/unzip_tool.py
LOCAL_MODULE_CLASS := ETC
LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/unzip_tool.py
include $(BUILD_PREBUILT)

######################################
# add logd.yaml to device, used for total entrace for slogd.yaml & sklogd.yaml & log-daemon.yaml
include $(CLEAR_VARS)
LOCAL_MODULE := log.yaml
ifeq ($(chip_id), hi1951)
LOCAL_SRC_FILES := ../etc/1951_mdc/log.yaml
else ifeq ($(chip_id), hi1951pg2)
LOCAL_SRC_FILES := ../etc/1951_mdc/log_pg2.yaml
endif
LOCAL_MODULE_CLASS := ETC
LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/log.yaml
include $(BUILD_PREBUILT)

# add slogd.yaml to device for mdc
include $(CLEAR_VARS)
LOCAL_MODULE := slogd.yaml
LOCAL_SRC_FILES := ../etc/1951_mdc/slogd.yaml
LOCAL_MODULE_CLASS := ETC
LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/slogd.yaml
include $(BUILD_PREBUILT)

######################################
endif

ifeq ($(product), lhisi)
######################################
# add set_up_slogd.sh for lhisi
include $(CLEAR_VARS)
LOCAL_MODULE := set_up_slogd.sh
ifeq ($(device_os),android)
LOCAL_SRC_FILES := ../../../scripts/lhisiAndroid/set_up_slogd.sh
else
LOCAL_SRC_FILES := ../../../scripts/lhisiLinux/set_up_slogd.sh
endif
LOCAL_MODULE_CLASS := ETC
LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/set_up_slogd.sh
include $(BUILD_PREBUILT)

endif
