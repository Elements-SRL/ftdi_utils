#include "usbdevicepresence.h"

#ifdef _WIN32

#include <windows.h> /*! Must be included before cfgmgr32.h */
#include <cfgmgr32.h>

#include <cwchar>
#include <cwctype>
#include <string>

#ifdef _MSC_VER
#pragma comment(lib, "cfgmgr32.lib")
#endif

/*! Defined in cfgmgr32.h only for recent Windows SDK targets: same value as the SDK */
#ifndef CM_GETIDLIST_FILTER_PRESENT
#define CM_GETIDLIST_FILTER_PRESENT (0x00000100)
#endif

/*! Asks Windows for the instance ids of the USB devices currently plugged in.
 *  The answer is a single buffer with all the ids one after the other, each terminated by '\0',
 *  and an extra '\0' at the end, e.g. "USB\VID_0403&PID_6010\FT1A2B3C\0USB\VID_046D...\0\0". */
static bool readPresentUsbDeviceIds(std::vector<wchar_t> &idBuffer) {
    const ULONG filter = CM_GETIDLIST_FILTER_ENUMERATOR | CM_GETIDLIST_FILTER_PRESENT;
    const int maxAttempts = 5;

    for (int attempt = 0; attempt < maxAttempts; attempt++) {
        /*! First ask how big the buffer must be, then fill it */
        ULONG length = 0;
        if (CM_Get_Device_ID_List_SizeW(&length, L"USB", filter) != CR_SUCCESS) {
            return false;
        }

        idBuffer.assign(length, L'\0');
        CONFIGRET result = CM_Get_Device_ID_ListW(L"USB", idBuffer.data(), length, filter);
        if (result == CR_SUCCESS) {
            return true;
        }

        /*! CR_BUFFER_SMALL: a device was plugged in between the two calls, try again */
        if (result != CR_BUFFER_SMALL) {
            return false;
        }
    }
    return false;
}

/*! Splits the buffer of readPresentUsbDeviceIds() into separate strings */
static std::vector<std::wstring> splitIds(const std::vector<wchar_t> &idBuffer) {
    std::vector<std::wstring> ids;
    size_t position = 0;
    while (position < idBuffer.size() && idBuffer[position] != L'\0') {
        std::wstring id(&idBuffer[position]); /*!< Copies up to the next '\0' */
        position += id.size() + 1;
        ids.push_back(id);
    }
    return ids;
}

/*! true if the instance id belongs to the vendor, e.g. "USB\VID_0403&PID_6010\..." for 0x0403 */
static bool idHasVendor(const std::wstring &id, uint16_t vendorId) {
    wchar_t pattern[16];
    swprintf(pattern, 16, L"VID_%04X", vendorId); /*!< e.g. L"VID_0403" */

    /*! Windows writes the ids in upper case, but do not rely on it */
    std::wstring upperCaseId = id;
    for (wchar_t &character : upperCaseId) {
        character = towupper(character);
    }
    return upperCaseId.find(pattern) != std::wstring::npos;
}

UsbPresence findUsbDeviceByVendorId(const std::vector<uint16_t> &vendorIds) {
    std::vector<wchar_t> idBuffer;
    if (!readPresentUsbDeviceIds(idBuffer)) {
        return UsbPresence::Unknown;
    }

    for (const std::wstring &id : splitIds(idBuffer)) {
        for (uint16_t vendorId : vendorIds) {
            if (idHasVendor(id, vendorId)) {
                return UsbPresence::Present;
            }
        }
    }
    return UsbPresence::Absent;
}

#elif __linux__

#include <filesystem>
#include <fstream>

/*! Linux lists the USB devices in /sys/bus/usb/devices: each device folder contains a file
 *  "idVendor" with the vendor id in hexadecimal (e.g. "0403"). */
UsbPresence findUsbDeviceByVendorId(const std::vector<uint16_t> &vendorIds) {
    const std::filesystem::path usbDevicesFolder("/sys/bus/usb/devices");

    std::error_code error;
    if (!std::filesystem::exists(usbDevicesFolder, error)) {
        return UsbPresence::Unknown;
    }

    for (const auto &entry : std::filesystem::directory_iterator(usbDevicesFolder, error)) {
        std::ifstream vendorFile(entry.path() / "idVendor");
        unsigned int deviceVendorId = 0;
        if (!(vendorFile >> std::hex >> deviceVendorId)) {
            continue; /*!< Not a device (e.g. an interface folder) */
        }

        for (uint16_t vendorId : vendorIds) {
            if (deviceVendorId == vendorId) {
                return UsbPresence::Present;
            }
        }
    }
    return UsbPresence::Absent;
}

#else

UsbPresence findUsbDeviceByVendorId(const std::vector<uint16_t> &) {
    return UsbPresence::Unknown;
}

#endif
