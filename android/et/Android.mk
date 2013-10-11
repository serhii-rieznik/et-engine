LOCAL_PATH := $(call my-dir)

INCLUDE_PATH := ../../include/
SOURCE_PATH := ../../src/

include $(CLEAR_VARS)

LOCAL_MODULE := et

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(INCLUDE_PATH)

LOCAL_CFLAGS := -UNDEBUG -DDEBUG
LOCAL_CXXFLAGS := --std=c++11 $(LOCAL_CFLAGS)

LOCAL_SRC_FILES += $(SOURCE_PATH)/networking/downloadmanager.cpp

LOCAL_SRC_FILES += $(SOURCE_PATH)/platform-android/stream.android.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/platform-android/application.android.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/platform-android/tools.android.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/platform-android/locale.android.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/platform-android/log.android.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/platform-android/rendercontext.android.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/platform-android/input.android.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/platform-android/charactergenerator.android.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/platform-android/sound.openal.android.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/platform-android/nativeactivity.android.cpp

LOCAL_SRC_FILES += $(SOURCE_PATH)/core/debug.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/core/plist.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/core/tools.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/core/objectscache.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/core/transformable.cpp

LOCAL_SRC_FILES += $(SOURCE_PATH)/scene3d/cameraelement.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/scene3d/element.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/scene3d/material.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/scene3d/mesh.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/scene3d/scene3d.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/scene3d/serialization.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/scene3d/storage.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/scene3d/supportmesh.cpp

LOCAL_SRC_FILES += $(SOURCE_PATH)/primitives/primitives.cpp

LOCAL_SRC_FILES += $(SOURCE_PATH)/locale/locale.cpp

LOCAL_SRC_FILES += $(SOURCE_PATH)/input/gestures.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/input/input.cpp

LOCAL_SRC_FILES += $(SOURCE_PATH)/camera/camera.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/camera/frustum.cpp

LOCAL_SRC_FILES += $(SOURCE_PATH)/gui/button.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/gui/carousel.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/gui/element2d.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/gui/element3d.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/gui/font.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/gui/gui.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/gui/guibase.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/gui/guirenderer.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/gui/imageview.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/gui/label.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/gui/layout.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/gui/listbox.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/gui/messageview.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/gui/renderingelement.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/gui/scroll.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/gui/slider.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/gui/textfield.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/gui/textureatlas.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/gui/textureatlaswriter.cpp

LOCAL_SRC_FILES += $(SOURCE_PATH)/imaging/ddsloader.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/imaging/imageoperations.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/imaging/imagewriter.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/imaging/jpgloader.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/imaging/pngloader.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/imaging/pvrloader.cpp

LOCAL_SRC_FILES += $(SOURCE_PATH)/tasks/taskpool.cpp

LOCAL_SRC_FILES += $(SOURCE_PATH)/timers/notifytimer.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/timers/sequence.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/timers/timedobject.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/timers/timerpool.cpp

LOCAL_SRC_FILES += $(SOURCE_PATH)/resources/textureloader.cpp

LOCAL_SRC_FILES += $(SOURCE_PATH)/apiobjects/framebufferdata.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/apiobjects/framebufferfactory.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/apiobjects/indexbufferdata.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/apiobjects/programdata.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/apiobjects/programfactory.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/apiobjects/texture.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/apiobjects/texturefactory.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/apiobjects/textureloadingthread.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/apiobjects/vertexarrayobjectdata.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/apiobjects/vertexbufferdata.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/apiobjects/vertexbufferfactory.cpp

LOCAL_SRC_FILES += $(SOURCE_PATH)/vertexbuffer/indexarray.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/vertexbuffer/vertexarray.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/vertexbuffer/vertexdatachunk.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/vertexbuffer/vertexdeclaration.cpp

LOCAL_SRC_FILES += $(SOURCE_PATH)/rendering/renderer.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/rendering/rendering.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/rendering/renderstate.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/rendering/rendercontext.cpp

LOCAL_SRC_FILES += $(SOURCE_PATH)/opengl/opengl.common.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/opengl/openglcaps.cpp

LOCAL_SRC_FILES += $(SOURCE_PATH)/app/appevironment.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/app/application.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/app/events.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/app/invocation.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/app/runloop.cpp

LOCAL_SRC_FILES += $(SOURCE_PATH)/collision/aabb.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/collision/collision.cpp

LOCAL_SRC_FILES += $(SOURCE_PATH)/platform-unix/criticalsection.unix.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/platform-unix/mutex.unix.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/platform-unix/atomiccounter.unix.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/platform-unix/thread.unix.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/platform-unix/threading.unix.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/platform-unix/tools.unix.cpp

LOCAL_SRC_FILES += $(SOURCE_PATH)/geometry/geometry.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/geometry/rectplacer.cpp

LOCAL_SRC_FILES += $(SOURCE_PATH)/sound/formats.cpp
LOCAL_SRC_FILES += $(SOURCE_PATH)/sound/sound.openal.cpp

LOCAL_STATIC_LIBRARIES := android_native_app_glue libpng libjpeg libzip libxml libcurl openal

include $(BUILD_STATIC_LIBRARY)

$(call import-add-path, $(LOCAL_PATH))
$(call import-module, android/native_app_glue)
$(call import-module, libpng)
$(call import-module, libjpeg)
$(call import-module, libzip)
$(call import-module, libxml)
$(call import-module, libcurl)
$(call import-module, openal)

