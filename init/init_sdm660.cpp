/*
   Copyright (c) 2016, The CyanogenMod Project
   Copyright (c) 2019, The LineageOS Project

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <vector>
#include <cstdlib>
#include <fstream>
#include <string.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#include <android-base/properties.h>
#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>

#include "vendor_init.h"
#include "property_service.h"

using android::base::GetProperty;
using std::string;

std::vector<std::string> ro_props_default_source_order = {
    "",
    "bootimage.",
    "odm.",
    "product.",
    "system.",
    "vendor.",
};

string heapstartsize, heapgrowthlimit, heapsize,
       heapminfree, heapmaxfree, heaptargetutilization;

void property_override(char const prop[], char const value[], bool add = true)
{
    prop_info *pi;

    pi = (prop_info *) __system_property_find(prop);
    if (pi)
        __system_property_update(pi, value, strlen(value));
    else if (add)
        __system_property_add(prop, strlen(prop), value, strlen(value));
}

void property_override_dual(char const system_prop[],
    char const vendor_prop[], char const value[])
{
    property_override(system_prop, value);
    property_override(vendor_prop, value);
}

void check_device()
{
    struct sysinfo sys;

    sysinfo(&sys);

    if (sys.totalram > 5072ull * 1024 * 1024) {
        // from - phone-xhdpi-6144-dalvik-heap.mk
        heapstartsize = "16m";
        heapgrowthlimit = "256m";
        heapsize = "512m";
        heaptargetutilization = "0.5";
        heapminfree = "8m";
        heapmaxfree = "32m";
    } else if (sys.totalram > 3072ull * 1024 * 1024) {
        // from - phone-xxhdpi-4096-dalvik-heap.mk
        heapstartsize = "8m";
        heapgrowthlimit = "256m";
        heapsize = "512m";
        heaptargetutilization = "0.6";
        heapminfree = "8m";
        heapmaxfree = "16m";
    } else {
        // from - phone-xhdpi-2048-dalvik-heap.mk
        heapstartsize = "8m";
        heapgrowthlimit = "192m";
        heapsize = "512m";
        heaptargetutilization = "0.75";
        heapminfree = "512k";
        heapmaxfree = "8m";
    }
}

void NFC_check()
{
    // Check NFC
    std::ifstream infile("/proc/NFC_CHECK");
    std::string check;

    getline(infile, check);
    if (!check.compare("SUPPORTED")) {
        property_override("ro.hq.support.nfc", "1");
        property_override("ro.boot.product.hardware.sku", "NFC");
    } else
        property_override("ro.hq.support.nfc", "0");
}

void set_ro_build_prop(const std::string &prop, const std::string &value) {
    for (const auto &source : ro_props_default_source_order) {
        auto prop_name = "ro." + source + "build." + prop;
        if (source == "")
            property_override(prop_name.c_str(), value.c_str());
        else
            property_override(prop_name.c_str(), value.c_str(), false);
    }
};

void set_ro_product_prop(const std::string &prop, const std::string &value) {
    for (const auto &source : ro_props_default_source_order) {
        auto prop_name = "ro.product." + source + prop;
        property_override(prop_name.c_str(), value.c_str(), false);
    }
};

void vendor_load_properties()
{
    check_device();
    NFC_check();

    std::string region;
    std::string hardware_revision;
    region = GetProperty("ro.boot.hwc", "GLOBAL");
    hardware_revision = GetProperty("ro.boot.hwversion", "UNKNOWN");

    std::string model;
    std::string device;
    std::string fingerprint;
    std::string description;
    std::string mod_device;
    std::string build_type;

    if (region == "GLOBAL") {
        model = "ZenFone Max Pro M1";
        device = "X00TD";
        fingerprint = "google/raven/raven:13/TP1A.220624.021/8877034:user/release-keys";
        description = "raven-user 13 TP1A.220624.021 8877034 release-keys";
        mod_device = "WW_X00TD";
    } else {
        model = "ZenFone Max Pro M1";
        device = "X00TD";
        fingerprint = "google/raven/raven:13/TP1A.220624.021/8877034:user/release-keys";
        description = "raven-user 13 TP1A.220624.021 8877034 release-keys";
        mod_device = "WW_X00TD";
    }

    set_ro_build_prop("fingerprint", fingerprint);
    set_ro_product_prop("device", device);
    set_ro_product_prop("model", model);
    property_override("ro.build.description", description.c_str());
    if (mod_device != "") {
        property_override("ro.product.mod_device", mod_device.c_str());
    }

    property_override_dual("ro.build.type", "ro.vendor.build.type", "user");
    property_override_dual("ro.odm.build.type", "ro.product.build.type", "user");
    property_override_dual("ro.system.build.type", "ro.system_ext.build.type", "user");

    property_override("ro.boot.hardware.revision", hardware_revision.c_str());
    property_override("ro.boot.verifiedbootstate", "green");

//    property_override("dalvik.vm.heapstartsize", heapstartsize);
//    property_override("dalvik.vm.heapgrowthlimit", heapgrowthlimit);
//    property_override("dalvik.vm.heapsize", heapsize);
//    property_override("dalvik.vm.heaptargetutilization", heaptargetutilization);
//    property_override("dalvik.vm.heapminfree", heapminfree);
//    property_override("dalvik.vm.heapmaxfree", heapmaxfree);
}
