
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

PRIVATE_SRC		:= ../../src
LOCAL_MODULE    := kxplayer
LOCAL_SRC_FILES := $(PRIVATE_SRC)/player.c \
					$(PRIVATE_SRC)/agent.c \
					$(PRIVATE_SRC)/device_android.c \
					boot.c \
					com_kaixindev_kxplayer_JNITest.c \
					com_kaixindev_kxplayer_Agent.c \
					com_kaixindev_kxplayer_Device.c 

LOCAL_C_INCLUDES := /Users/kaixin/Work/RadioPlayer/build-android/include
LOCAL_LDLIBS := -llog -lz -lavformat -lavcodec -lavutil -losapi -L/Users/kaixin/Work/RadioPlayer/build-android/lib
#LOCAL_CFLAGS := -g

include $(BUILD_SHARED_LIBRARY)

