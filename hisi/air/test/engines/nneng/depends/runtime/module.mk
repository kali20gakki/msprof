LOCAL_PATH := $(call my-dir)

LOCAL_C_INCLUDES := \
    $(TOPDIR)inc \
    $(TOPDIR)metadef/inc \
    $(TOPDIR)graphengine/inc \
    $(TOPDIR)inc/external \
    $(TOPDIR)metadef/inc/external \
    $(TOPDIR)graphengine/inc/external \
    $(TOPDIR)inc/external/graph \
    $(TOPDIR)metadef/inc/external/graph \
    $(TOPDIR)graphengine/inc/external/graph \
    $(TOPDIR)graphengine/inc/framework \
    $(TOPDIR)inc/cce \
    $(TOPDIR)libc_sec/include \
    $(TOPDIR)graphengine/ge/ \
    $(TOPDIR)graphengine/ge/common \
    $(TOPDIR)third_party/glog/include \
    $(TOPDIR)third_party/protobuf/include \

LOCAL_SRC_FILES := \
    src/runtime_stub.cpp \

#compile x86 libruntime_stub
include $(CLEAR_VARS)

LOCAL_MODULE := libruntime_stub_fe

LOCAL_CFLAGS += -Dgoogle=ascend_private

LOCAL_C_INCLUDES := \
    $(TOPDIR)inc \
    $(TOPDIR)metadef/inc \
    $(TOPDIR)graphengine/inc \
    $(TOPDIR)inc/external \
    $(TOPDIR)metadef/inc/external \
    $(TOPDIR)graphengine/inc/external \
    $(TOPDIR)inc/external/graph \
    $(TOPDIR)metadef/inc/external/graph \
    $(TOPDIR)graphengine/inc/external/graph \
    $(TOPDIR)graphengine/inc/framework \
    $(TOPDIR)inc/cce \
    $(TOPDIR)libc_sec/include \
    $(TOPDIR)graphengine/ge/ \
    $(TOPDIR)graphengine/ge/common \
    $(TOPDIR)third_party/glog/include \
    $(TOPDIR)third_party/protobuf/include \

LOCAL_SRC_FILES := \
    src/runtime_stub.cpp \

include $(BUILD_LLT_SHARED_LIBRARY)
