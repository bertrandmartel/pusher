LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := buttonpusher

LOCAL_CFLAGS := -std=gnu++11
LOCAL_CFLAGS += -funwind-tables -Wl,--no-merge-exidx-entries
LOCAL_CPPFLAGS += -fexceptions

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../aes
LOCAL_C_INCLUDES += $NDK/sources/cxx-stl/gnu-libstdc++/4.8/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)

LOCAL_SRC_FILES  := buttonpusher_wrapper.cpp

#aes

AES_PATH := ../aes

LOCAL_SRC_FILES += $(AES_PATH)/AES.cpp

LOCAL_LDLIBS    := -llog

include $(BUILD_SHARED_LIBRARY)
