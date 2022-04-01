
LOCAL_PATH := $(call my-dir)

LOCAL_CPP_FILES_SUFFIX := %.cpp %.cc

LOCAL_AU_CPP_FILES := $(foreach src_path,$(LOCAL_PATH), $(shell find "$(src_path)" -type f))

LOCAL_AU_CPP_FILES := $(LOCAL_AU_CPP_FILES:$(LOCAL_PATH)/./%=$(LOCAL_PATH)%)

COMM_SRC_FILES  := $(filter $(LOCAL_CPP_FILES_SUFFIX),$(LOCAL_AU_CPP_FILES))

COMM_SRC_FILES  := $(COMM_SRC_FILES:$(LOCAL_PATH)/%=%)

COMMON_LOCAL_SRC_FILES := $(COMM_SRC_FILES) \

COMMON_LOCAL_C_INCLUDES := $(LOCAL_PATH) \
                           inc \
                           inc/external \
                           inc/external/runtime \
                           metadef/inc \
                           graphengine/inc \
                           metadef/inc/common \
                           metadef/inc/external/  \
                           graphengine/inc/external/  \
                           metadef/inc/framework/ \
                           inc/toolchain/ \
                           metadef/inc/register/ \
                           fusion_engine/inc/ \
                           third_party/json/include/ \
                           libc_sec/include/ \
                           metadef/graph/ \
                           proto/task.proto \
                           proto/ge_ir.proto \
                           third_party/protobuf/include \

local_aicoreutil_shared_library := libc_sec libslog libgraph libascend_protobuf liberror_manager libresource libregister libruntime

local_atc_aicoreutil_shared_library := libc_sec libslog libgraph libascend_protobuf liberror_manager libresource libregister libruntime_compile

#compiler for host
include $(CLEAR_VARS)
LOCAL_MODULE := libaicore_utils

LOCAL_CFLAGS += -Werror -Dgoogle=ascend_private

LOCAL_CPPFLAGS += -fexceptions

LOCAL_C_INCLUDES := $(COMMON_LOCAL_C_INCLUDES)

LOCAL_SRC_FILES  := $(COMMON_LOCAL_SRC_FILES)

LOCAL_SHARED_LIBRARIES := $(local_aicoreutil_shared_library)

LOCAL_LDFLAGS := -ldl

LOCAL_MULTILIB := 64

LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_HOST_SHARED_LIBRARY)

#compiler for host atc
include $(CLEAR_VARS)
LOCAL_MODULE := atclib/libaicore_utils

LOCAL_CFLAGS += -Werror -O2 -DCOMPILE_OMG_PACKAGE -Dgoogle=ascend_private

LOCAL_CPPFLAGS += -fexceptions

LOCAL_C_INCLUDES := $(COMMON_LOCAL_C_INCLUDES)

LOCAL_SRC_FILES  := $(COMMON_LOCAL_SRC_FILES)

LOCAL_SHARED_LIBRARIES := $(local_atc_aicoreutil_shared_library)

LOCAL_LDFLAGS := -ldl

LOCAL_MULTILIB := 64

LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_HOST_SHARED_LIBRARY)

#compiler for host acl
include $(CLEAR_VARS)
LOCAL_MODULE := libaicore_utils_runtime

LOCAL_CFLAGS += -Werror -Dgoogle=ascend_private

LOCAL_CPPFLAGS += -fexceptions

LOCAL_C_INCLUDES := $(COMMON_LOCAL_C_INCLUDES)

LOCAL_SRC_FILES  := $(COMMON_LOCAL_SRC_FILES)

LOCAL_SHARED_LIBRARIES := $(local_aicoreutil_shared_library)

LOCAL_LDFLAGS := -ldl

LOCAL_MULTILIB := 64

LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_HOST_SHARED_LIBRARY)

#compiler for device acl
include $(CLEAR_VARS)
LOCAL_MODULE := libaicore_utils_runtime

LOCAL_CFLAGS += -Werror -Dgoogle=ascend_private

LOCAL_CPPFLAGS += -fexceptions

LOCAL_C_INCLUDES := $(COMMON_LOCAL_C_INCLUDES)

LOCAL_SRC_FILES  := $(COMMON_LOCAL_SRC_FILES)

LOCAL_SHARED_LIBRARIES := $(local_aicoreutil_shared_library)

LOCAL_LDFLAGS := -ldl

LOCAL_MULTILIB := 64

LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

#compiler for device
include $(CLEAR_VARS)
LOCAL_MODULE := libaicore_utils

LOCAL_CFLAGS += -Werror -O2 -Dgoogle=ascend_private

LOCAL_C_INCLUDES := $(COMMON_LOCAL_C_INCLUDES)

LOCAL_SRC_FILES  := $(COMMON_LOCAL_SRC_FILES)

LOCAL_SHARED_LIBRARIES := $(local_aicoreutil_shared_library)

LOCAL_LDFLAGS := -ldl

LOCAL_MULTILIB := 64

LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

#compiler for host static(.a)
include $(CLEAR_VARS)
LOCAL_MODULE := libaicore_utils

LOCAL_CFLAGS += -Werror -O2 -Dgoogle=ascend_private

LOCAL_CPPFLAGS += -fexceptions

LOCAL_C_INCLUDES := $(COMMON_LOCAL_C_INCLUDES)

LOCAL_SRC_FILES  := $(COMMON_LOCAL_SRC_FILES)

LOCAL_SHARED_LIBRARIES := $(local_aicoreutil_shared_library)

LOCAL_LDFLAGS := -ldl

LOCAL_MULTILIB := 64

LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_HOST_STATIC_LIBRARY)

#compiler for device static(.a)
include $(CLEAR_VARS)
LOCAL_MODULE := libaicore_utils

LOCAL_CFLAGS += -Werror -O2 -Dgoogle=ascend_private

LOCAL_C_INCLUDES := $(COMMON_LOCAL_C_INCLUDES)

LOCAL_SRC_FILES  := $(COMMON_LOCAL_SRC_FILES)

LOCAL_SHARED_LIBRARIES := $(local_aicoreutil_shared_library)

LOCAL_LDFLAGS := -ldl

LOCAL_MULTILIB := 64

LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_STATIC_LIBRARY)

# compile for ut/st
include $(CLEAR_VARS)
LOCAL_MODULE := libaicore_utils

LOCAL_CFLAGS += -Werror -Dgoogle=ascend_private

LOCAL_C_INCLUDES := $(COMMON_LOCAL_C_INCLUDES)

LOCAL_SRC_FILES  := $(COMMON_LOCAL_SRC_FILES)

LOCAL_SHARED_LIBRARIES := $(local_aicoreutil_shared_library)

LOCAL_LDFLAGS := -ldl

LOCAL_MULTILIB := 64
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_LLT_SHARED_LIBRARY)
