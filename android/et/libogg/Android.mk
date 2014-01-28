LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libogg

LOCAL_C_INCLUDES = $(LOCAL_PATH)/../../../include

LOCAL_CFLAGS += -I$(LOCAL_PATH)/../include -ffast-math -fsigned-char

ifeq ($(TARGET_ARCH),arm)
	LOCAL_CFLAGS += -marm -mfloat-abi=softfp -mfpu=vfp
endif


LOCAL_SRC_FILES := \
	bitwise.c \
	framing.c

include $(BUILD_STATIC_LIBRARY)
