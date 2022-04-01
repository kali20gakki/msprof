LOCAL_PATH := $(call my-dir)
FE_SRC_PATH := $(TOPDIR)fusion_engine/optimizer
FE_LLT_PATH := $(TOPDIR)llt/fusion_engine
CM_SRC_PATH := $(TOPDIR)fusion_engine/utils
OK_SRC_PATH := $(TOPDIR)fusion_engine/opskernel

LOCAL_CPP_FILES_SUFFIX := %.cpp %.cc

LOCAL_FE_CPP_FILES := $(foreach src_path,$(FE_SRC_PATH), $(shell find "$(src_path)" -type f))
LOCAL_FE_CPP_FILES := $(LOCAL_FE_CPP_FILES:$(FE_SRC_PATH)/./%=$(FE_SRC_PATH)%)
FE_SRC_FILES  := $(filter $(LOCAL_CPP_FILES_SUFFIX),$(LOCAL_FE_CPP_FILES))
FE_SRC_FILES  := $(FE_SRC_FILES:$(FE_SRC_PATH)/%=../../../$(FE_SRC_PATH)/%)

LOCAL_CM_CPP_FILES := $(foreach src_path,$(CM_SRC_PATH), $(shell find "$(src_path)" -type f))
LOCAL_CM_CPP_FILES := $(LOCAL_CM_CPP_FILES:$(CM_SRC_PATH)/./%=$(CM_SRC_PATH)%)
CM_SRC_FILES  := $(filter $(LOCAL_CPP_FILES_SUFFIX),$(LOCAL_CM_CPP_FILES))
CM_SRC_FILES  := $(CM_SRC_FILES:$(CM_SRC_PATH)/%=../../../$(CM_SRC_PATH)/%)

LOCAL_OK_CPP_FILES := $(foreach src_path,$(OK_SRC_PATH), $(shell find "$(src_path)" -type f))
LOCAL_OK_CPP_FILES := $(LOCAL_OK_CPP_FILES:$(OK_SRC_PATH)/./%=$(OK_SRC_PATH)%)
OK_SRC_FILES  := $(filter $(LOCAL_CPP_FILES_SUFFIX),$(LOCAL_OK_CPP_FILES))
OK_SRC_FILES  := $(OK_SRC_FILES:$(OK_SRC_PATH)/%=../../../$(OK_SRC_PATH)/%)

LOCAL_FE_UT_CPP_FILES := $(foreach ut_path,$(LOCAL_PATH), $(shell find "$(ut_path)" -type f))
LOCAL_FE_UT_CPP_FILES := $(LOCAL_FE_UT_CPP_FILES:$(LOCAL_PATH)/./%=$(LOCAL_PATH)%)
FE_UT_FILES  := $(filter $(LOCAL_CPP_FILES_SUFFIX),$(LOCAL_FE_UT_CPP_FILES))
FE_UT_FILES  := $(FE_UT_FILES:$(LOCAL_PATH)/%=%)

FE_C_INCLUDES := $(LOCAL_PATH) \
                  $(FE_SRC_PATH) \
                  $(CM_SRC_PATH) \
                  $(OK_SRC_PATH) \
                  $(TOPDIR)third_party/json/include \
                  $(TOPDIR)third_party/protobuf/include \
                  $(TOPDIR)libc_sec/include \
                  $(TOPDIR)metadef/inc/common/opskernel \
                  $(TOPDIR)metadef/inc/common/optimizer \
                  $(TOPDIR)metadef/inc/common/util \
                  $(TOPDIR)metadef/inc/common \
                  $(TOPDIR)metadef/third_party \
                  $(TOPDIR)metadef/third_party/ \
                  $(TOPDIR)inc/external \
                  $(TOPDIR)metadef/inc/external \
                  $(TOPDIR)graphengine/inc/external \
                  $(TOPDIR)metadef/inc/external/graph \
                  $(TOPDIR)graphengine/inc/framework \
                  $(TOPDIR)inc/tensor_engine \
                  $(TOPDIR)inc/toolchain \
                  $(TOPDIR)metadef/inc/graph/utils \
                  $(TOPDIR)inc/cce/optimizer/tiling \
                  $(TOPDIR)inc/cce/optimizer \
                  $(TOPDIR)inc/cce/graph/utils \
                  $(TOPDIR)inc/cce/graph \
                  $(TOPDIR)inc/cce/common \
                  $(TOPDIR)inc/cce \
                  $(TOPDIR)inc \
                  $(TOPDIR)metadef/inc \
                  $(TOPDIR)graphengine/inc \
                  $(TOPDIR)metadef \
                  $(TOPDIR)inc/fusion_engine/ \
                  $(TOPDIR)fusion_engine/inc \
                  $(TOPDIR)cann/ops/built-in/op_proto/inc \
                  $(TOPDIR)metadef/inc/graph/ \
                  $(TOPDIR)llt/fusion_engine/graph_constructor \
                  $(TOPDIR)metadef/inc/register \

COMMON_FE_FILES := proto/om.proto \
                    proto/task.proto \
                    proto/insert_op.proto \
                    proto/ge_ir.proto \

FE_FILES := $(FE_SRC_FILES) \
            $(CM_SRC_FILES) \
            $(OK_SRC_FILES) \
            $(FE_UT_FILES) \

STUB_FILES := $(foreach src_path,$(FE_LLT_PATH)/graph_constructor, $(shell find "$(src_path)" -type f))
GRAPH_CONSTRUCTOR_FILES  := $(filter $(LOCAL_CPP_FILES_SUFFIX),$(STUB_FILES))
GRAPH_CONSTRUCTOR_FILES  := $(GRAPH_CONSTRUCTOR_FILES:$(FE_LLT_PATH)/%=../%)
###########################fusion_engine_utest_new###########################
include $(CLEAR_VARS)

LOCAL_CLASSFILE_RULE := Fusion_Engine_Ascend
LOCAL_INC_COV_BLACKLIST := $(LOCAL_PATH)/fe_ut_inc_blacklist
LOCAL_INC_COV_THRESHOLD := 0

LOCAL_MODULE := fe_ut

LOCAL_SRC_FILES := $(COMMON_FE_FILES) \
                   $(FE_FILES) \
                   $(GRAPH_CONSTRUCTOR_FILES) \
                   ../stub/slog_stub.cc \

LOCAL_C_INCLUDES := $(FE_C_INCLUDES)

LOCAL_SHARED_LIBRARIES := libmmpa libgraph_stub_fe libregister_stub_fe libresource_stub_fe libruntime_stub_fe libascend_protobuf libplatform libcompress liberror_manager

LOCAL_CFLAGS += -D__OPTIMIZER_KT_TEST__ -DDAVINCI_CLOUD -Dgoogle=ascend_private

LOCAL_STATIC_LIBRARIES := libc_sec

include $(BUILD_UT_TEST)
