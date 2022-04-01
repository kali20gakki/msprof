############################################################################
# host libadcore.a
#
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libadcore

LOCAL_SRC_FILES := ../../hdc-common/hdc_api.cpp \
                   ../../hdc-common/ide_hdc_interface.cpp \
                   ../../hdc-common/device/adx_dsmi.cpp \
                   ../../hdc-common/device/adx_device.cpp \
                   ../../hdc-common/device/adx_hdc_device.cpp \
                   ../../hdc-common/component/adx_server_manager.cpp \
                   ../../hdc-common/component/adx_file_dump.cpp \
                   ../../hdc-common/component/adx_get_file.cpp \
                   ../../hdc-common/epoll/adx_hdc_epoll.cpp \
                   ../../hdc-common/commopts/adx_comm_opt_manager.cpp \
                   ../../hdc-common/commopts/hdc_comm_opt.cpp \
                   ../../hdc-common/common/thread.cpp \
                   ../../hdc-common/common/memory_utils.cpp \
                   ../../hdc-common/protocol/adx_msg_proto.cpp \
                   ../../external/adx_core_dump_server.cpp \
                   ../../external/adx_get_file_server.cpp \
                   ../../hdc-common/common/file_utils.cpp \
                   ../../hdc-common/ide_process_util.cpp \
                   ../../external/adx_set_log_level.cpp \
                   ../../external/adx_log_api.cpp \
                   ../../external/adx_api.cpp \

LOCAL_C_INCLUDES := ${LOCAL_PATH}/../../../../../libc_sec/include \
                    ${LOCAL_PATH}/../../hdc-common \
                    ${LOCAL_PATH}/../../hdc-common/epoll \
                    ${LOCAL_PATH}/../../hdc-common/manager \
                    ${LOCAL_PATH}/../../hdc-common/manager/component \
                    ${LOCAL_PATH}/../../hdc-common/commopts \
                    ${LOCAL_PATH}/../../hdc-common/common \
                    ${LOCAL_PATH}/../../hdc-common/component \
                    ${LOCAL_PATH}/../../hdc-common/protocol \
                    ${LOCAL_PATH}/../../external \
                    ${LOCAL_PATH}/../../../../../inc/driver \
                    ${LOCAL_PATH}/../../../../../inc/toolchain \
                    ${LOCAL_PATH}/../../../../../inc/mmpa \

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

LOCAL_CFLAGS += -Werror -std=c++11 -DADX_LIB_HOST_DRV -DOS_TYPE=0 #OS_TYPE LINUX=0 WIN=1
LOCAL_LDFLAGS += -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack
LOCAL_SHARED_LIBRARIES := libc_sec libmmpa libslog libascend_hal
include $(BUILD_HOST_STATIC_LIBRARY)

############################################################################
# device libadcore.a
#
include $(CLEAR_VARS)

LOCAL_MODULE := libadcore

LOCAL_SRC_FILES := ../../hdc-common/hdc_api.cpp \
                   ../../hdc-common/ide_hdc_interface.cpp \
                   ../../hdc-common/device/adx_dsmi.cpp \
                   ../../hdc-common/device/adx_device.cpp \
                   ../../hdc-common/device/adx_hdc_device.cpp \
                   ../../hdc-common/component/adx_server_manager.cpp \
                   ../../hdc-common/component/adx_file_dump.cpp \
                   ../../hdc-common/component/adx_get_file.cpp \
                   ../../hdc-common/epoll/adx_hdc_epoll.cpp \
                   ../../hdc-common/commopts/adx_comm_opt_manager.cpp \
                   ../../hdc-common/commopts/hdc_comm_opt.cpp \
                   ../../hdc-common/common/thread.cpp \
                   ../../hdc-common/common/memory_utils.cpp \
                   ../../hdc-common/protocol/adx_msg_proto.cpp \
                   ../../external/adx_core_dump_server.cpp \
                   ../../external/adx_get_file_server.cpp \
                   ../../hdc-common/common/file_utils.cpp \
                   ../../hdc-common/ide_process_util.cpp \
                   ../../external/adx_api.cpp \

LOCAL_C_INCLUDES := ${LOCAL_PATH}/../../../../../libc_sec/include \
                    ${LOCAL_PATH}/../../hdc-common \
                    ${LOCAL_PATH}/../../hdc-common/epoll \
                    ${LOCAL_PATH}/../../hdc-common/manager \
                    ${LOCAL_PATH}/../../hdc-common/manager/component \
                    ${LOCAL_PATH}/../../hdc-common/commopts \
                    ${LOCAL_PATH}/../../hdc-common/common \
                    ${LOCAL_PATH}/../../hdc-common/component \
                    ${LOCAL_PATH}/../../hdc-common/protocol \
                    ${LOCAL_PATH}/../../external \
                    ${LOCAL_PATH}/../../../../../inc/driver \
                    ${LOCAL_PATH}/../../../../../inc/toolchain \
                    ${LOCAL_PATH}/../../../../../inc/mmpa \
LOCAL_CFLAGS := -Werror -std=c++11 -fstack-protector-strong -DADX_LIB -DOS_TYPE=0 #OS_TYPE LINUX=0 WIN=1
LOCAL_LDFLAGS += -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack

LOCAL_SHARED_LIBRARIES := libc_sec libmmpa libslog
include $(BUILD_STATIC_LIBRARY)
############################################################################
############################################################################
# device liblog_server.a
#
include $(CLEAR_VARS)

LOCAL_MODULE := liblog_server

LOCAL_SRC_FILES := ../../hdc-common/hdc_api.cpp \
                   ../../hdc-common/ide_hdc_interface.cpp \
                   ../../hdc-common/device/adx_dsmi.cpp \
                   ../../hdc-common/device/adx_device.cpp \
                   ../../hdc-common/device/adx_hdc_device.cpp \
                   ../../hdc-common/component/adx_server_manager.cpp \
                   ../../hdc-common/component/adx_log_hdc.cpp \
                   ../../hdc-common/epoll/adx_hdc_epoll.cpp \
                   ../../hdc-common/commopts/adx_comm_opt_manager.cpp \
                   ../../hdc-common/commopts/hdc_comm_opt.cpp \
                   ../../hdc-common/common/thread.cpp \
                   ../../hdc-common/common/memory_utils.cpp \
                   ../../hdc-common/protocol/adx_msg_proto.cpp \
                   ../../external/adx_log_hdc_server.cpp \
                   ../../hdc-common/common/file_utils.cpp \
                   ../../hdc-common/ide_process_util.cpp \
                   ../../external/adx_api.cpp \

LOCAL_C_INCLUDES := ${LOCAL_PATH}/../../../../../libc_sec/include \
                    ${LOCAL_PATH}/../../hdc-common \
                    ${LOCAL_PATH}/../../hdc-common/epoll \
                    ${LOCAL_PATH}/../../hdc-common/manager \
                    ${LOCAL_PATH}/../../hdc-common/manager/component \
                    ${LOCAL_PATH}/../../hdc-common/commopts \
                    ${LOCAL_PATH}/../../hdc-common/common \
                    ${LOCAL_PATH}/../../hdc-common/component \
                    ${LOCAL_PATH}/../../hdc-common/protocol \
                    ${LOCAL_PATH}/../../external \
                    ${LOCAL_PATH}/../../../../../inc/driver \
                    ${LOCAL_PATH}/../../../../../inc/toolchain \
                    ${LOCAL_PATH}/../../../../../inc/mmpa \
                    ${LOCAL_PATH}/../../../../../toolchain/log/shared \
                    ${LOCAL_PATH}/../../../../../toolchain/log/slog/slog/src \

ifeq ($(product), lhisi)
LOCAL_SRC_FILES := ../../external/adx_log_hdc_server_stub.cpp \

LOCAL_C_INCLUDES := ${LOCAL_PATH}/../../external \
                    ${LOCAL_PATH}/../../hdc-common \
else ifeq($(chip_id), hi1951)
LOCAL_SRC_FILES := ../../external/adx_log_hdc_server_stub.cpp \

LOCAL_C_INCLUDES := ${LOCAL_PATH}/../../external \
                    ${LOCAL_PATH}/../../hdc-common \
else
LOCAL_SHARED_LIBRARIES := libc_sec libmmpa libslog libascend_hal
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

LOCAL_CFLAGS += -Werror -std=c++11 -DADX_LIB_HOST -DSESSION_ACTIVE -DOS_TYPE=0 #OS_TYPE LINUX=0 WIN=1
LOCAL_LDFLAGS += -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack

include $(BUILD_STATIC_LIBRARY)


# host libadump_server.a
#
include $(CLEAR_VARS)

LOCAL_MODULE := libadump_server

LOCAL_SRC_FILES := ../../hdc-common/commopts/sock_api.cpp \
                   ../../hdc-common/component/adx_server_manager.cpp \
                   ../../hdc-common/component/dump/adx_dump_receive.cpp \
                   ../../hdc-common/commopts/adx_comm_opt_manager.cpp \
                   ../../hdc-common/commopts/sock_comm_opt.cpp \
                   ../../hdc-common/device/adx_device.cpp \
                   ../../hdc-common/device/adx_sock_device.cpp \
                   ../../hdc-common/common/thread.cpp \
                   ../../hdc-common/common/memory_utils.cpp \
                   ../../hdc-common/common/file_utils.cpp \
                   ../../hdc-common/ide_process_util.cpp \
                   ../../hdc-common/common/string_utils.cpp \
                   ../../hdc-common/protocol/adx_msg_proto.cpp \
                   ../../hdc-common/component/dump/adx_dump_process.cpp \
                   ../../hdc-common/component/dump/adx_dump_record.cpp

LOCAL_C_INCLUDES := ${LOCAL_PATH}/../../../../../libc_sec/include \
                    ${LOCAL_PATH}/../../hdc-common \
                    ${LOCAL_PATH}/../../hdc-common/log \
                    ${LOCAL_PATH}/../../hdc-common/epoll \
                    ${LOCAL_PATH}/../../hdc-common/component \
                    ${LOCAL_PATH}/../../hdc-common/component/dump \
                    ${LOCAL_PATH}/../../hdc-common/commopts \
                    ${LOCAL_PATH}/../../hdc-common/common \
                    ${LOCAL_PATH}/../../hdc-common/device \
                    ${LOCAL_PATH}/../../hdc-common/protocol \
                    ${LOCAL_PATH}/../../external \
                    ${LOCAL_PATH}/../../../../../inc/driver \
                    ${LOCAL_PATH}/../../../../../inc/toolchain \
                    ${LOCAL_PATH}/../../../../../inc/mmpa \

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

SOC_SRC_FILES := ../../external/adx_datadump_server_soc.cpp \
                ../../hdc-common/epoll/adx_sock_epoll.cpp

ifeq ($(product), lhisi)
LOCAL_CFLAGS += -DIDE_UNIFY_HOST_DEVICE
LOCAL_SRC_FILES += ${SOC_SRC_FILES}

else ifeq ($(rc), mini)
LOCAL_CFLAGS += -DIDE_UNIFY_HOST_DEVICE
LOCAL_SRC_FILES += ${SOC_SRC_FILES}

else ifeq (${product}, mdc)
LOCAL_CFLAGS += -DIDE_UNIFY_HOST_DEVICE
LOCAL_SRC_FILES += ${SOC_SRC_FILES}

else
LOCAL_SRC_FILES += ../../hdc-common/hdc_api.cpp \
                   ../../hdc-common/ide_hdc_interface.cpp \
                   ../../hdc-common/device/adx_dsmi.cpp \
                   ../../hdc-common/device/adx_device.cpp \
                   ../../hdc-common/device/adx_hdc_device.cpp \
                   ../../hdc-common/commopts/hdc_comm_opt.cpp \
                   ../../hdc-common/epoll/adx_hdc_epoll.cpp \
                   ../../external/adx_datadump_callback.cpp \
                   ../../external/adx_datadump_server.cpp
endif

LOCAL_CFLAGS += -Werror -std=c++11 -DADX_LIB_HOST -DOS_TYPE=0 #OS_TYPE LINUX=0 WIN=1
LOCAL_LDFLAGS += -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack
LOCAL_SHARED_LIBRARIES := libc_sec libmmpa libslog
LOCAL_UNINSTALLABLE_MODULE := false
include $(BUILD_HOST_STATIC_LIBRARY)

############################################################################
# host libadump_server_stub.a
include $(CLEAR_VARS)

LOCAL_MODULE := libadump_server_stub
LOCAL_SRC_FILES := ../../external/adx_datadump_server_stub.cpp \
LOCAL_C_INCLUDES := ${LOCAL_PATH}/../../external \
                    ${LOCAL_PATH}/../../hdc-common \

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

LOCAL_CFLAGS += -Werror -std=c++11 -DOS_TYPE=0 #OS_TYPE LINUX=0 WIN=1
LOCAL_LDFLAGS += -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack

include $(BUILD_HOST_STATIC_LIBRARY)

############################################################################
# device libadump_server.a
#
include $(CLEAR_VARS)

LOCAL_MODULE := libadump_server

LOCAL_SRC_FILES := ../../hdc-common/commopts/sock_api.cpp \
                   ../../hdc-common/component/adx_server_manager.cpp \
                   ../../hdc-common/component/dump/adx_dump_receive.cpp \
                   ../../hdc-common/commopts/adx_comm_opt_manager.cpp \
                   ../../hdc-common/commopts/sock_comm_opt.cpp \
                   ../../hdc-common/device/adx_device.cpp \
                   ../../hdc-common/device/adx_sock_device.cpp \
                   ../../hdc-common/common/thread.cpp \
                   ../../hdc-common/common/memory_utils.cpp \
                   ../../hdc-common/common/file_utils.cpp \
				   ../../hdc-common/ide_process_util.cpp \
                   ../../hdc-common/common/string_utils.cpp \
                   ../../hdc-common/protocol/adx_msg_proto.cpp \
                   ../../hdc-common/component/dump/adx_dump_process.cpp \
                   ../../hdc-common/component/dump/adx_dump_record.cpp

LOCAL_C_INCLUDES := ${LOCAL_PATH}/../../../../../libc_sec/include \
                    ${LOCAL_PATH}/../../hdc-common \
                    ${LOCAL_PATH}/../../hdc-common/log \
                    ${LOCAL_PATH}/../../hdc-common/epoll \
                    ${LOCAL_PATH}/../../hdc-common/component \
                    ${LOCAL_PATH}/../../hdc-common/component/dump \
                    ${LOCAL_PATH}/../../hdc-common/commopts \
                    ${LOCAL_PATH}/../../hdc-common/common \
                    ${LOCAL_PATH}/../../hdc-common/device \
                    ${LOCAL_PATH}/../../hdc-common/protocol \
                    ${LOCAL_PATH}/../../external \
                    ${LOCAL_PATH}/../../../../../inc/driver \
                    ${LOCAL_PATH}/../../../../../inc/toolchain \
                    ${LOCAL_PATH}/../../../../../inc/mmpa \

SOC_SRC_FILES := ../../external/adx_datadump_server_soc.cpp \
                ../../hdc-common/epoll/adx_sock_epoll.cpp

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

LOCAL_CFLAGS += -DIDE_UNIFY_HOST_DEVICE
LOCAL_SRC_FILES += ${SOC_SRC_FILES}
LOCAL_CFLAGS := -Werror -std=c++11 -DADX_LIB_HOST -DOS_TYPE=0 #OS_TYPE LINUX=0 WIN=1
LOCAL_LDFLAGS += -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack

LOCAL_SHARED_LIBRARIES := libc_sec libmmpa libslog
LOCAL_UNINSTALLABLE_MODULE := false
include $(BUILD_STATIC_LIBRARY)

############################################################################
# device libadump_server_stub.a
include $(CLEAR_VARS)

LOCAL_MODULE := libadump_server_stub
LOCAL_SRC_FILES := ../../external/adx_datadump_server_stub.cpp \

LOCAL_C_INCLUDES := ${LOCAL_PATH}/../../external \
                    ${LOCAL_PATH}/../../hdc-common \

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

LOCAL_CFLAGS += -Werror -std=c++11 -DOS_TYPE=0 #OS_TYPE LINUX=0 WIN=1
LOCAL_LDFLAGS += -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack

include $(BUILD_STATIC_LIBRARY)
