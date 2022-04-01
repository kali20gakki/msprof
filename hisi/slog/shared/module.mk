LOCAL_PATH := $(call my-dir)

src_files := cfg_file_parse.c \
             print_log.c

inc_files := abl/libc_sec/include \
             inc/toolchain \
             ${LOCAL_PATH}

slog_tool_files := log_zip.c 
slog_tool_src := toolchain/log/shared \
                 toolchain/log/slog/slog/etc \
                 inc/toolchain \
                 abl/libc_sec/include \
                 third_party/zlib/zlib-1.2.11 \

######################################
include $(CLEAR_VARS)

LOCAL_MODULE := libparse-conf

LOCAL_SRC_FILES := $(src_files)

LOCAL_C_INCLUDES := $(inc_files)

#LOCAL_CFLAGS :=

#LOCAL_LDFLAGS :=

LOCAL_CFLAGS := -Werror -DOS_TYPE_DEF=0

LOCAL_SHARED_LIBRARIES := libc_sec

include $(BUILD_HOST_STATIC_LIBRARY)

#######################################

include $(CLEAR_VARS)

LOCAL_MODULE := libparse-conf

LOCAL_SRC_FILES := $(src_files)

LOCAL_C_INCLUDES := $(inc_files)

LOCAL_CFLAGS := -Werror -DOS_TYPE_DEF=0

#LOCAL_LDFLAGS :=

LOCAL_SHARED_LIBRARIES := libc_sec

include $(BUILD_STATIC_LIBRARY)

########################################
include $(CLEAR_VARS)

LOCAL_MODULE := libparse-conf

LOCAL_SRC_FILES := $(src_files)

LOCAL_C_INCLUDES := $(inc_files)

LOCAL_CFLAGS := -Werror -DOS_TYPE_DEF=0

#LOCAL_LDFLAGS :=

LOCAL_SHARED_LIBRARIES := libc_sec

include $(BUILD_LLT_STATIC_LIBRARY)
#####################################

include $(CLEAR_VARS)

zip_src_files := log_zip.c

zip_inc_files := abl/libc_sec/include \
             ${LOCAL_PATH}  \
             third_party/zlib/zlib-1.2.11 \


LOCAL_MODULE := liblog-zip

LOCAL_SRC_FILES := $(zip_src_files)

LOCAL_C_INCLUDES := $(zip_inc_files)

LOCAL_CFLAGS := -Werror

#LOCAL_LDFLAGS :=

LOCAL_SHARED_LIBRARIES := libc_sec
LOCAL_STATIC_LIBRARIES := libzlib

include $(BUILD_STATIC_LIBRARY)

#########################################
include $(CLEAR_VARS)
LOCAL_MODULE := slog_tool

LOCAL_SRC_FILES := $(slog_tool_files)

LOCAL_C_INCLUDES := $(slog_tool_src)

LOCAL_CFLAGS := -Werror -DSLOG_TOOLS

#LOCAL_LDFLAGS :=

LOCAL_SHARED_LIBRARIES := libc_sec libslog
LOCAL_STATIC_LIBRARIES := libzlib

include $(BUILD_HOST_EXECUTABLE)
