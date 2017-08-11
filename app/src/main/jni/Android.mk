LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_LDLIBS    := -llog
LOCAL_MODULE := IsNetOk
LOCAL_SRC_FILES := NativeMethod.c
include $(BUILD_SHARED_LIBRARY)
LOCAL_CFLAGS=-g
LOCAL_CERTIFICATE := platform