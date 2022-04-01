
LOCAL_PATH := $(call my-dir)

LOCAL_CPP_FILES_SUFFIX := %.cpp %.cc

LOCAL_FE_CPP_FILES := $(foreach src_path,$(LOCAL_PATH), $(shell find "$(src_path)" -type f))

LOCAL_FE_CPP_FILES := $(LOCAL_FE_CPP_FILES:$(LOCAL_PATH)/./%=$(LOCAL_PATH)%)

FE_SRC_FILES  := $(filter $(LOCAL_CPP_FILES_SUFFIX),$(LOCAL_FE_CPP_FILES))

FE_SRC_FILES  := $(FE_SRC_FILES:$(LOCAL_PATH)/%=%)

local_lib_fe_src_files := $(FE_SRC_FILES) \

local_lib_fe_inc_dirs := $(LOCAL_PATH) \
                         proto/task.proto \
                         proto/ge_ir.proto \
                         third_party/protobuf/include \
                         third_party/json/include \
                         metadef/inc/common/opskernel/ \
                         metadef/inc/common/optimizer/ \
                         metadef/inc/common/util/ \
                         inc/tensor_engine/ \
                         inc/toolchain/ \
                         libc_sec/include/ \
                         inc/cce/optimizer/ \
                         inc/cce/optimizer/tiling/ \
                         inc/cce/common/ \
                         inc/cce/ \
                         metadef/inc/framework/ \
                         graphengine/inc/framework/ \
                         inc/ \
                         graphengine/inc/ \
                         metadef/inc/ \
                         metadef/inc/external/ \
                         graphengine/inc/external/ \
                         metadef/inc/external/graph/ \
                         metadef/inc/external/framework/ \
                         inc/fusion_engine/ \
                         metadef/inc/graph/ \
                         metadef/inc/register/ \
                         fusion_engine/inc/ \

local_fe_shared_library := libc_sec libslog libruntime libgraph libascend_protobuf libresource libregister libplatform libcompress liberror_manager libaicore_utils libopskernel

local_atc_fe_shared_library := libc_sec libslog libruntime_compile libgraph libascend_protobuf libresource libregister libplatform libcompress liberror_manager libaicore_utils libopskernel

#compiler for host
include $(CLEAR_VARS)

LOCAL_MODULE := libfe

LOCAL_SRC_FILES := $(local_lib_fe_src_files)

LOCAL_C_INCLUDES := $(local_lib_fe_inc_dirs)

LOCAL_SHARED_LIBRARIES := $(local_fe_shared_library)

LOCAL_CFLAGS += -Werror -Dgoogle=ascend_private -Wno-deprecated-declarations

LOCAL_LDFLAGS := -ldl

LOCAL_MULTILIB := 64
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_HOST_SHARED_LIBRARY)

# add fe.ini to host
include $(CLEAR_VARS)

LOCAL_MODULE := fe.ini

LOCAL_SRC_FILES := fe_config/fe.ini

LOCAL_MODULE_CLASS := ETC

LOCAL_INSTALLED_PATH := $(HOST_OUT_ROOT)/fe.ini
include $(BUILD_HOST_PREBUILT)

# add fusion_config.json to host
include $(CLEAR_VARS)

LOCAL_MODULE := fusion_config.json

LOCAL_SRC_FILES := fe_config/fusion_config.json

LOCAL_MODULE_CLASS := ETC

LOCAL_INSTALLED_PATH := $(HOST_OUT_ROOT)/fusion_config.json
include $(BUILD_HOST_PREBUILT)

#compiler for device
include $(CLEAR_VARS)

LOCAL_MODULE := libfe

LOCAL_SRC_FILES := $(local_lib_fe_src_files)

LOCAL_C_INCLUDES := $(local_lib_fe_inc_dirs)

LOCAL_SHARED_LIBRARIES := $(local_fe_shared_library)

LOCAL_CFLAGS += -Werror -Dgoogle=ascend_private -Wno-deprecated-declarations

LOCAL_LDFLAGS := -ldl

LOCAL_MULTILIB := 64
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

# add fe.ini to device
include $(CLEAR_VARS)

LOCAL_MODULE := fe.ini

LOCAL_SRC_FILES := fe_config/fe.ini

LOCAL_MODULE_CLASS := ETC

LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/fe.ini
include $(BUILD_PREBUILT)

# add fusion_config.json to device
include $(CLEAR_VARS)

LOCAL_MODULE := fusion_config.json

LOCAL_SRC_FILES := fe_config/fusion_config.json

LOCAL_MODULE_CLASS := ETC

LOCAL_INSTALLED_PATH := $(DEVICE_OUT_ROOT)/fusion_config.json
include $(BUILD_PREBUILT)

#compiler for host
include $(CLEAR_VARS)

LOCAL_MODULE := atclib/libfe

LOCAL_SRC_FILES := $(local_lib_fe_src_files)

LOCAL_C_INCLUDES := $(local_lib_fe_inc_dirs)

LOCAL_SHARED_LIBRARIES := $(local_atc_fe_shared_library)

LOCAL_CFLAGS += -Werror -DCOMPILE_OMG_PACKAGE -Dgoogle=ascend_private -Wno-deprecated-declarations

LOCAL_LDFLAGS := -ldl

LOCAL_MULTILIB := 64
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_HOST_SHARED_LIBRARY)
