
LOCAL_PATH := $(call my-dir)

LOCAL_CPP_FILES_SUFFIX := %.cc

LOCAL_OPSKERNEL_CPP_FILES := $(foreach src_path,$(LOCAL_PATH), $(shell find "$(src_path)" -type f))

LOCAL_OPSKERNEL_CPP_FILES := $(LOCAL_OPSKERNEL_CPP_FILES:$(LOCAL_PATH)/./%=$(LOCAL_PATH)%)

OPSKERNEL_SRC_FILES  := $(filter $(LOCAL_CPP_FILES_SUFFIX),$(LOCAL_OPSKERNEL_CPP_FILES))

OPSKERNEL_SRC_FILES  := $(OPSKERNEL_SRC_FILES:$(LOCAL_PATH)/%=%)

OPSKERNEL_LOCAL_SRC_FILES := $(OPSKERNEL_SRC_FILES) \

OPSPERNEL_LOCAL_C_INCLUDES := $(LOCAL_PATH) \
                              inc \
                              graphengine/inc \
                              metadef/inc \
                              metadef/inc/common \
                              graphengine/inc/external/  \
                              metadef/inc/external/  \
                              inc/toolchain/ \
                              metadef/inc/register/ \
                              metadef/inc/framework/ \
                              fusion_engine/inc/ \
                              libc_sec/include/ \
                              metadef//graph/ \
                              third_party/json/include/ \

local_opskernel_shared_library := libc_sec libslog libgraph liberror_manager libaicore_utils

#compiler for host
include $(CLEAR_VARS)
LOCAL_MODULE := libopskernel

LOCAL_CFLAGS += -Werror

LOCAL_CPPFLAGS += -fexceptions

LOCAL_C_INCLUDES := $(OPSPERNEL_LOCAL_C_INCLUDES)

LOCAL_SRC_FILES  := $(OPSKERNEL_LOCAL_SRC_FILES)

LOCAL_SHARED_LIBRARIES := $(local_opskernel_shared_library)

LOCAL_LDFLAGS := -ldl

LOCAL_MULTILIB := 64

LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_HOST_SHARED_LIBRARY)

#compiler for device
include $(CLEAR_VARS)
LOCAL_MODULE := libopskernel

LOCAL_CFLAGS += -Werror -O2

LOCAL_C_INCLUDES := $(OPSPERNEL_LOCAL_C_INCLUDES)

LOCAL_SRC_FILES  := $(OPSKERNEL_LOCAL_SRC_FILES)

LOCAL_SHARED_LIBRARIES := $(local_opskernel_shared_library)

LOCAL_LDFLAGS := -ldl

LOCAL_MULTILIB := 64

LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

