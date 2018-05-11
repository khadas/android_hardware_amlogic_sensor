# Copyright (C) 2011 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

ifeq ($(strip $(BOARD_ALSA_AUDIO)),tiny)

    LOCAL_PATH := $(call my-dir)

# The default audio HAL module, which is a stub, that is loaded if no other
# device specific modules are present. The exact load order can be seen in
# libhardware/hardware.c
#
# The format of the name is audio.<type>.<hardware/etc>.so where the only
# required type is 'primary'. Other possibilites are 'a2dp', 'usb', etc.
    include $(CLEAR_VARS)

    LOCAL_MODULE := audio.primary.amlogic
    ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
        LOCAL_PROPRIETARY_MODULE := true
    endif
    LOCAL_MODULE_RELATIVE_PATH := hw
    LOCAL_SRC_FILES := \
        audio_hw.c \
        audio_hw_utils.c \
        audio_hwsync.c \
        audio_hw_profile.c \
        aml_hw_mixer.c \
        alsa_manager.c \
        audio_hw_ms12.c \
        aml_audio_stream.c \
        alsa_config_parameters.c \
        spdif_encoder_api.c \
        audio_eq_drc_compensation.cpp \
        audio_eq_drc_parser.cpp \
        aml_ac3_parser.c \
        aml_dcv_dec_api.c \
        alsa_device_parser.c \
        audio_post_process.c \
        audio_format_parse.c

    LOCAL_C_INCLUDES += \
        external/tinyalsa/include \
        system/media/audio_utils/include \
        system/media/audio_effects/include \
        system/media/audio_route/include \
        system/core/include \
        $(LOCAL_PATH)/../utils/include \
        $(LOCAL_PATH)/../utils/ini/include \
        $(LOCAL_PATH)/../rcaudio

    LOCAL_SHARED_LIBRARIES := \
        liblog libcutils libtinyalsa \
        libaudioutils libdl libaudioroute libutils \
        libaudiospdif libamaudioutils libamlaudiorc

    LOCAL_MODULE_TAGS := optional

    LOCAL_CFLAGS += -Werror
    #LOCAL_CFLAGS += -Wall -Wunknown-pragmas

#add dolby ms12support
ifeq ($(strip $(DOLBY_MS12_ENABLE)), true)
    LOCAL_SHARED_LIBRARIES += libms12api
    LOCAL_CFLAGS += -DDOLBY_MS12_ENABLE
    LOCAL_CFLAGS += -DREPLACE_OUTPUT_BUFFER_WITH_CALLBACK
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libms12/include
endif

    include $(BUILD_SHARED_LIBRARY)

endif # BOARD_ALSA_AUDIO

#########################################################
# Audio Policy Manager
ifeq ($(USE_CUSTOM_AUDIO_POLICY),1)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    DLGAudioPolicyManager.cpp

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    liblog \
    libutils \
    libmedia \
    libbinder \
    libaudiopolicymanagerdefault \
    libutils \
    libaudioclient \
    libmedia_helper

LOCAL_C_INCLUDES := \
    external/tinyalsa/include \
    $(TOPDIR)frameworks/av/services/audiopolicy \
    $(TOPDIR)frameworks/av/services/audiopolicy/managerdefault \
    $(TOPDIR)frameworks/av/services/audiopolicy/engine/interface \
    $(TOPDIR)frameworks/av/services/audiopolicy/common/managerdefinitions/include \
    $(TOPDIR)frameworks/av/services/audiopolicy/common/include \
    $(TOPDIR)frameworks/av/media/libaudioclient/include

LOCAL_MODULE := libaudiopolicymanager
LOCAL_MODULE_TAGS := optional
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
    LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)
endif # USE_CUSTOM_AUDIO_POLICY

include $(call all-makefiles-under,$(LOCAL_PATH))