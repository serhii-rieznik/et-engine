LOCAL_PATH := $(call my-dir)/../../

SOURCE_PATH := source
ENGINE_INCLUDES_PATH := $(LOCAL_PATH)../include/

include $(CLEAR_VARS)

LOCAL_MODULE := demo

LOCAL_C_INCLUDES := $(ENGINE_INCLUDES_PATH)

LOCAL_CPPFLAGS += --std=c++11

LOCAL_SRC_FILES += \
	$(SOURCE_PATH)/main.cpp \
	$(SOURCE_PATH)/maincontroller.cpp \
	$(SOURCE_PATH)/ui/mainmenu.cpp \
	$(SOURCE_PATH)/ui/resourcemanager.cpp

LOCAL_LDLIBS := -llog -landroid -lz -lGLESv2
LOCAL_STATIC_LIBRARIES := android_native_app_glue et png

include $(BUILD_SHARED_LIBRARY)

$(call import-add-path, $(LOCAL_PATH)../android)
$(call import-module, android/native_app_glue)
$(call import-module, et)
