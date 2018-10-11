/*
// Copyright (c) 2014 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This file is modified by Amlogic, Inc. 2017.01.17.
*/

#ifndef PRIMARY_DEVICE_H
#define PRIMARY_DEVICE_H

#include <utils/KeyedVector.h>
#include <PhysicalDevice.h>
#include <IComposeDeviceFactory.h>

#define DEVICE_STR_MBOX                 "MBOX"
#define DEVICE_STR_TV                   "TV"

//#if PLATFORM_SDK_VERSION >= 26 //8.0
//#define DISPLAY_CFG_FILE                "/vendor/etc/mesondisplay.cfg"
//#else
//#define DISPLAY_CFG_FILE                "/system/etc/mesondisplay.cfg"
//#endif

namespace android {
namespace amlogic {


class PrimaryDevice : public PhysicalDevice {
public:
    PrimaryDevice(Hwcomposer& hwc, IComposeDeviceFactory *composer);
    virtual ~PrimaryDevice();
public:
    virtual bool initialize();
    virtual void deinitialize();
    virtual int32_t createVirtualDisplay(uint32_t width __unused, uint32_t height __unused, int32_t* format __unused, hwc2_display_t* outDisplay __unused){ return HWC2_ERROR_NONE; }
    virtual int32_t destroyVirtualDisplay(hwc2_display_t display __unused) { return HWC2_ERROR_NONE; }
    virtual int32_t setOutputBuffer(buffer_handle_t buffer __unused, int32_t releaseFence __unused) { return HWC2_ERROR_NONE; }

private:

    static void hotplugEventListener(void *data, bool status);
    static void modeChangeEventListener(void *data, bool status);
    void hotplugListener(bool connected, bool modeSwitch);
    int parseConfigFile();

    const char* pConfigPath;
    int mDisplayType;
};

}
}

#endif /* PRIMARY_DEVICE_H */
