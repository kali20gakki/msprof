LOCAL_PATH := $(call my-dir)

sklogd_src_files := ../src/klogd.c \
                   ../../common/common.c \
                   ../../../shared/start_single_process.c \
                   ../../../shared/log_sys_package.c \
                   ../../../shared/print_log.c \
                   ../../slog/src/log_monitor.c

sklogd_inc_files := ${LOCAL_PATH}/../../../../../abl/libc_sec/include ${LOCAL_PATH}/../../include \
                    ${LOCAL_PATH}/../ \
                    ${LOCAL_PATH}/../../common \
                    ${LOCAL_PATH}/../../../shared \
                    ${LOCAL_PATH}/../../slog/src \
                    inc/driver \
                    inc/toolchain

#####################################
# build sklogd device
include $(CLEAR_VARS)
LOCAL_MODULE := sklogd

ifeq ($(product), cloud)
    LOCAL_CFLAGS += -D_LOG_MONITOR_ -D_DEV_CLOUD_
    LOCAL_SHARED_LIBRARIES += libascend_monitor
else ifeq ($(product), mini)
    ifneq ($(rc), mini)
        LOCAL_CFLAGS += -D_LOG_MONITOR_
        LOCAL_SHARED_LIBRARIES += libascend_monitor
    endif
else ifeq ($(product)_$(chip_id), mdc_hi1951pg2)
    #no iam featurefor 1951 mdc pg2
else ifeq ($(product)_$(chip_id), mdc_hi1951)
    LOCAL_LDFLAGS += -L${MDC_LINUX_SDK_DIR}/lib64 \
                     -L${MDC_LINUX_SDK_DIR}/lib64/mdc/base-plat \
                     -liam -leasy_comm -lxshmem
    sklogd_inc_files += ${MDC_LINUX_SDK_DIR}/include
    LOCAL_CFLAGS += -DIAM
endif

LOCAL_SRC_FILES := $(sklogd_src_files)
LOCAL_C_INCLUDES := $(sklogd_inc_files)

LOCAL_CFLAGS += -Werror -fstack-protector-strong -fPIE -pie -D_FORTIFY_SOURCE=2 -O2 -DOS_TYPE_DEF=0 -D_SLOG_DEVICE_
LOCAL_LDFLAGS += -lpthread -lc_sec -Wl,-z,relro -Wl,-z,noexecstack -fPIE -pie -Wl,-z,now
LOCAL_SHARED_LIBRARIES += libc_sec libslog
include $(BUILD_EXECUTABLE)

######################################
# add sklogd_daemon_monitor.sh to device
include $(CLEAR_VARS)
LOCAL_MODULE := sklogd_daemon_monitor.sh
LOCAL_SRC_FILES := ../../../scripts/sklogd_daemon_monitor.sh
LOCAL_MODULE_CLASS := ETC
LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/sklogd_daemon_monitor.sh
include $(BUILD_PREBUILT)

ifeq ($(product)_$(chip_id), mdc_hi1951)
######################################
# add sklogd.yaml to device for mdc
include $(CLEAR_VARS)
LOCAL_MODULE := sklogd.yaml
LOCAL_SRC_FILES := ../../slog/etc/1951_mdc/sklogd.yaml
LOCAL_MODULE_CLASS := ETC
LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/sklogd.yaml
include $(BUILD_PREBUILT)
######################################
endif

ifeq ($(product)_$(chip_id), mdc_hi1951pg2)
######################################
# add sklogd.yaml to device for mdc pg2
include $(CLEAR_VARS)
LOCAL_MODULE := sklogd.yaml
LOCAL_SRC_FILES := ../../slog/etc/1951_mdc/sklogd.yaml
LOCAL_MODULE_CLASS := ETC
LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/sklogd.yaml
include $(BUILD_PREBUILT)
######################################
endif
