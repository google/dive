LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)

LOCAL_MODULE := libwrap
LOCAL_SRC_FILES := libwrap.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE	:= libVkLayer_dive_capture
LOCAL_SRC_FILES	:= dispatch.cc  layer_base.cc layer_impl.cc  vk_layer_android.cc gles_layer.cc 
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../third_party/Vulkan-Headers/include
LOCAL_SHARED_LIBRARIES := libwrap
LOCAL_LDLIBS := -llog -lc -ldl

include $(BUILD_SHARED_LIBRARY)