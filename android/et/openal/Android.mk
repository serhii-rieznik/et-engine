LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# LOCAL_C_INCLUDES := $(LOCAL_PATH)/include $(LOCAL_PATH)/openal/include
# LOCAL_ARM_MODE := arm

LOCAL_MODULE := openal

LOCAL_SRC_FILES := alAuxEffectSlot.c alBuffer.c alDatabuffer.c alEffect.c alError.c alExtension.c \
	alFilter.c alListener.c alSource.c alState.c alThunk.c ALc.c alcConfig.c alcEcho.c\
	alcModulator.c alcReverb.c alcRing.c alcThread.c ALu.c android.cpp bs2b.c null.c 

LOCAL_CFLAGS := -DAL_BUILD_LIBRARY -DAL_ALEXT_PROTOTYPES

include $(BUILD_STATIC_LIBRARY)