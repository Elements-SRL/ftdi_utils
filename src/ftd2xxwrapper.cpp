#include "ftd2xxwrapper.h"

#include <cstring>
#include <mutex>

#include "internal/ftdilibraries.h"
#include "internal/handleregistry.h"
#include "internal/usbdevicepresence.h"

namespace {

/*! Serializes the operations that are not tied to a handle (list, open, close).
 *  FTD2XX is not safe when the device list is read and modified by different threads. */
std::mutex globalMutex;

/*! Vendor ids for isFtdiDevicePresent(), protected by their own mutex */
std::mutex vendorIdsMutex;
std::vector<uint16_t> usbVendorIds = {0x0403}; /*!< FTDI */

/*! Returned by every function when the library it needs is not loaded */
constexpr FT_STATUS libraryNotLoaded = FT_OTHER_ERROR;

} // namespace

// ============================================================================
// Libraries management
// ============================================================================

bool Ftd2xxWrapper::loadFtd2xx() {
    return FtdiLibraries::loadFtd2xx();
}

bool Ftd2xxWrapper::loadMpsse() {
    return FtdiLibraries::loadMpsse();
}

bool Ftd2xxWrapper::isFtd2xxLoaded() {
    return FtdiLibraries::isFtd2xxLoaded();
}

bool Ftd2xxWrapper::isMpsseLoaded() {
    return FtdiLibraries::isMpsseLoaded();
}

bool Ftd2xxWrapper::unloadLibraries() {
    /*! globalMutex: no thread can open a new handle while the libraries are unloaded */
    std::lock_guard<std::mutex> lock(globalMutex);

    if (!HandleRegistry::noOpenHandles()) {
        return false;
    }
    FtdiLibraries::unload();
    return true;
}

std::string Ftd2xxWrapper::getLastLoadError() {
    return FtdiLibraries::lastErrorMessage();
}

FtdiLoadError Ftd2xxWrapper::getLastLoadErrorType() {
    return FtdiLibraries::lastErrorType();
}

void Ftd2xxWrapper::setLibraryDirectory(const std::filesystem::path &directory) {
    FtdiLibraries::setLibraryDirectory(directory);
}

// ============================================================================
// Device presence check
// ============================================================================

bool Ftd2xxWrapper::isFtdiDevicePresent() {
    std::vector<uint16_t> vendorIds;
    {
        std::lock_guard<std::mutex> lock(vendorIdsMutex);
        vendorIds = usbVendorIds;
    }

    if (vendorIds.empty()) {
        return true; /*!< Check disabled */
    }

    switch (findUsbDeviceByVendorId(vendorIds)) {
    case UsbPresence::Present:
        return true;

    case UsbPresence::Absent:
        return false;

    case UsbPresence::Unknown:
    default:
        return true; /*!< Cannot tell: let FTD2XX decide */
    }
}

void Ftd2xxWrapper::setUsbVendorIds(const std::vector<uint16_t> &vendorIds) {
    std::lock_guard<std::mutex> lock(vendorIdsMutex);
    usbVendorIds = vendorIds;
}

// ============================================================================
// Global operations (no handle)
// ============================================================================

FT_STATUS Ftd2xxWrapper::FTW_ListDevices(PVOID pArg1, PVOID pArg2, DWORD Flags) {
    std::lock_guard<std::mutex> lock(globalMutex);
    if (!FtdiLibraries::loadFtd2xx()) {
        return libraryNotLoaded;
    }
    return FtdiLibraries::ftd2xx().ListDevices(pArg1, pArg2, Flags);
}

FT_STATUS Ftd2xxWrapper::FTW_CreateDeviceInfoList(LPDWORD numDevs) {
    std::lock_guard<std::mutex> lock(globalMutex);
    if (!FtdiLibraries::loadFtd2xx()) {
        return libraryNotLoaded;
    }
    return FtdiLibraries::ftd2xx().CreateDeviceInfoList(numDevs);
}

FT_STATUS Ftd2xxWrapper::FTW_GetDeviceInfoList(FT_DEVICE_LIST_INFO_NODE* list, LPDWORD numDevs) {
    std::lock_guard<std::mutex> lock(globalMutex);
    if (!FtdiLibraries::loadFtd2xx()) {
        return libraryNotLoaded;
    }
    return FtdiLibraries::ftd2xx().GetDeviceInfoList(list, numDevs);
}

FT_STATUS Ftd2xxWrapper::FTW_Open(int deviceNumber, FT_HANDLE* handle) {
    std::lock_guard<std::mutex> lock(globalMutex);
    if (!FtdiLibraries::loadFtd2xx()) {
        return libraryNotLoaded;
    }

    FT_STATUS status = FtdiLibraries::ftd2xx().Open(deviceNumber, handle);
    if (status == FT_OK) {
        HandleRegistry::addOpenHandle(*handle);
    }
    return status;
}

FT_STATUS Ftd2xxWrapper::FTW_OpenEx(PVOID pArg1, DWORD flags, FT_HANDLE* handle) {
    std::lock_guard<std::mutex> lock(globalMutex);
    if (!FtdiLibraries::loadFtd2xx()) {
        return libraryNotLoaded;
    }

    FT_STATUS status = FtdiLibraries::ftd2xx().OpenEx(pArg1, flags, handle);
    if (status == FT_OK) {
        HandleRegistry::addOpenHandle(*handle);
    }
    return status;
}

FT_STATUS Ftd2xxWrapper::FTW_Close(FT_HANDLE handle) {
    std::lock_guard<std::mutex> lock(globalMutex);
    if (!FtdiLibraries::isFtd2xxLoaded()) {
        return libraryNotLoaded;
    }

    FT_STATUS status = FtdiLibraries::ftd2xx().Close(handle);
    if (status == FT_OK) {
        HandleRegistry::removeHandle(handle);
    }
    return status;
}

void Ftd2xxWrapper::SPIW_Init_libMPSSE() {
    std::lock_guard<std::mutex> lock(globalMutex);
    if (!FtdiLibraries::loadMpsse()) {
        return;
    }

    /*! Optional function: not exported by every libMPSSE version */
    if (FtdiLibraries::mpsse().Init != nullptr) {
        FtdiLibraries::mpsse().Init();
    }
}

void Ftd2xxWrapper::SPIW_Cleanup_libMPSSE() {
    std::lock_guard<std::mutex> lock(globalMutex);
    if (!FtdiLibraries::isMpsseLoaded()) {
        return;
    }

    /*! Optional function: not exported by every libMPSSE version */
    if (FtdiLibraries::mpsse().Cleanup != nullptr) {
        FtdiLibraries::mpsse().Cleanup();
    }
}

FT_STATUS Ftd2xxWrapper::SPIW_OpenChannel(DWORD index, FT_HANDLE *handle) {
    std::lock_guard<std::mutex> lock(globalMutex);
    if (!FtdiLibraries::loadMpsse()) {
        return libraryNotLoaded;
    }

    FT_STATUS status = FtdiLibraries::mpsse().OpenChannel(index, handle);
    if (status == FT_OK) {
        HandleRegistry::addOpenHandle(*handle);
    }
    return status;
}

FT_HANDLE Ftd2xxWrapper::SPIW_OpenChannelBySerial(std::string serial) {
    std::lock_guard<std::mutex> lock(globalMutex);
    if (!FtdiLibraries::loadMpsse()) {
        return nullptr;
    }
    const MpsseFunctions &mpsse = FtdiLibraries::mpsse();

    /*! 1. Get the current number of SPI channels */
    DWORD numChannels = 0;
    FT_STATUS status = mpsse.GetNumChannels(&numChannels);
    if (status != FT_OK || numChannels == 0) {
        return nullptr;
    }

    /*! 2. Look for the channel with the requested serial number and open it */
    for (DWORD channelIdx = 0; channelIdx < numChannels; channelIdx++) {
        FT_DEVICE_LIST_INFO_NODE channelInfo;
        status = mpsse.GetChannelInfo(channelIdx, &channelInfo);
        if (status != FT_OK) {
            continue;
        }

        if (strcmp(channelInfo.SerialNumber, serial.c_str()) != 0) {
            continue;
        }

        FT_HANDLE handle = nullptr;
        status = mpsse.OpenChannel(channelIdx, &handle);
        if (status == FT_OK) {
            HandleRegistry::addOpenHandle(handle);
            return handle;
        }
    }

    return nullptr;
}

FT_STATUS Ftd2xxWrapper::SPIW_CloseChannel(FT_HANDLE handle) {
    std::lock_guard<std::mutex> lock(globalMutex);
    if (!FtdiLibraries::isMpsseLoaded()) {
        return libraryNotLoaded;
    }

    FT_STATUS status = FtdiLibraries::mpsse().CloseChannel(handle);
    if (status == FT_OK) {
        HandleRegistry::removeHandle(handle);
    }
    return status;
}

// ============================================================================
// Handle-based operations
// A valid handle means that the library is loaded (unloadLibraries() refuses to
// unload while handles are open): the "is loaded" checks only protect against misuse.
// ============================================================================

FT_STATUS Ftd2xxWrapper::FTW_SetBitMode(FT_HANDLE handle, UCHAR ucMask, UCHAR ucEnable) {
    if (!FtdiLibraries::isFtd2xxLoaded()) {
        return libraryNotLoaded;
    }
    HandleLock lock(handle);
    return FtdiLibraries::ftd2xx().SetBitMode(handle, ucMask, ucEnable);
}

FT_STATUS Ftd2xxWrapper::FTW_SetLatencyTimer(FT_HANDLE handle, UCHAR ucLatency) {
    if (!FtdiLibraries::isFtd2xxLoaded()) {
        return libraryNotLoaded;
    }
    HandleLock lock(handle);
    return FtdiLibraries::ftd2xx().SetLatencyTimer(handle, ucLatency);
}

FT_STATUS Ftd2xxWrapper::FTW_SetUSBParameters(FT_HANDLE handle, ULONG ulInTransferSize, ULONG ulOutTransferSize) {
    if (!FtdiLibraries::isFtd2xxLoaded()) {
        return libraryNotLoaded;
    }
    HandleLock lock(handle);
    return FtdiLibraries::ftd2xx().SetUSBParameters(handle, ulInTransferSize, ulOutTransferSize);
}

FT_STATUS Ftd2xxWrapper::FTW_SetFlowControl(FT_HANDLE handle, USHORT FlowControl, UCHAR XonChar, UCHAR XoffChar) {
    if (!FtdiLibraries::isFtd2xxLoaded()) {
        return libraryNotLoaded;
    }
    HandleLock lock(handle);
    return FtdiLibraries::ftd2xx().SetFlowControl(handle, FlowControl, XonChar, XoffChar);
}

FT_STATUS Ftd2xxWrapper::FTW_GetQueueStatus(FT_HANDLE handle, DWORD *dwRxBytes) {
    if (!FtdiLibraries::isFtd2xxLoaded()) {
        return libraryNotLoaded;
    }
    HandleLock lock(handle);
    return FtdiLibraries::ftd2xx().GetQueueStatus(handle, dwRxBytes);
}

FT_STATUS Ftd2xxWrapper::FTW_Write(FT_HANDLE handle, LPVOID buffer, DWORD bytesToWrite, LPDWORD bytesWritten) {
    if (!FtdiLibraries::isFtd2xxLoaded()) {
        return libraryNotLoaded;
    }
    HandleLock lock(handle);
    return FtdiLibraries::ftd2xx().Write(handle, buffer, bytesToWrite, bytesWritten);
}

FT_STATUS Ftd2xxWrapper::FTW_Read(FT_HANDLE handle, LPVOID buffer, DWORD bytesToRead, LPDWORD bytesRead) {
    if (!FtdiLibraries::isFtd2xxLoaded()) {
        return libraryNotLoaded;
    }
    HandleLock lock(handle);
    return FtdiLibraries::ftd2xx().Read(handle, buffer, bytesToRead, bytesRead);
}

FT_STATUS Ftd2xxWrapper::FTW_Purge(FT_HANDLE handle, ULONG mask) {
    if (!FtdiLibraries::isFtd2xxLoaded()) {
        return libraryNotLoaded;
    }
    HandleLock lock(handle);
    return FtdiLibraries::ftd2xx().Purge(handle, mask);
}

FT_STATUS Ftd2xxWrapper::FTW_ResetDevice(FT_HANDLE handle) {
    if (!FtdiLibraries::isFtd2xxLoaded()) {
        return libraryNotLoaded;
    }
    HandleLock lock(handle);
    return FtdiLibraries::ftd2xx().ResetDevice(handle);
}

FT_STATUS Ftd2xxWrapper::FTW_ReadEE(FT_HANDLE handle, DWORD wordOffset, LPWORD value) {
    if (!FtdiLibraries::isFtd2xxLoaded()) {
        return libraryNotLoaded;
    }
    HandleLock lock(handle);
    return FtdiLibraries::ftd2xx().ReadEE(handle, wordOffset, value);
}

FT_STATUS Ftd2xxWrapper::SPIW_InitChannel(FT_HANDLE handle, ChannelConfig *config) {
    if (!FtdiLibraries::isMpsseLoaded()) {
        return libraryNotLoaded;
    }
    HandleLock lock(handle);
    return FtdiLibraries::mpsse().InitChannel(handle, config);
}

FT_STATUS Ftd2xxWrapper::SPIW_Read(FT_HANDLE handle, UCHAR *buffer,
                                   DWORD sizeToTransfer, LPDWORD sizeTransfered, DWORD options) {
    if (!FtdiLibraries::isMpsseLoaded()) {
        return libraryNotLoaded;
    }
    HandleLock lock(handle);
    return FtdiLibraries::mpsse().Read(handle, buffer, sizeToTransfer, sizeTransfered, options);
}

FT_STATUS Ftd2xxWrapper::SPIW_Write(FT_HANDLE handle, UCHAR *buffer,
                                    DWORD sizeToTransfer, LPDWORD sizeTransfered, DWORD options) {
    if (!FtdiLibraries::isMpsseLoaded()) {
        return libraryNotLoaded;
    }
    HandleLock lock(handle);
    return FtdiLibraries::mpsse().Write(handle, buffer, sizeToTransfer, sizeTransfered, options);
}

FT_STATUS Ftd2xxWrapper::FTW_WriteGPIO(FT_HANDLE handle, UCHAR dir, UCHAR value) {
    if (!FtdiLibraries::isMpsseLoaded()) {
        return libraryNotLoaded;
    }
    HandleLock lock(handle);
    return FtdiLibraries::mpsse().WriteGPIO(handle, dir, value);
}

FT_STATUS Ftd2xxWrapper::FTW_ReadGPIO(FT_HANDLE handle, UCHAR *value) {
    if (!FtdiLibraries::isMpsseLoaded()) {
        return libraryNotLoaded;
    }
    HandleLock lock(handle);
    return FtdiLibraries::mpsse().ReadGPIO(handle, value);
}

void Ftd2xxWrapper::CleanupHandle(FT_HANDLE handle) {
    HandleRegistry::removeHandle(handle);
}

// ============================================================================
// Higher level utilities
// ============================================================================

int32_t Ftd2xxWrapper::getDeviceIndex(std::string serial) {
    DWORD numDevs;
    if (!getDeviceCount(numDevs)) {
        return -1;
    }

    for (uint32_t index = 0; index < numDevs; index++) {
        if (getDeviceSerial(index, false) == serial) {
            return static_cast<int32_t>(index);
        }
    }
    return -1;
}

std::string Ftd2xxWrapper::getDeviceSerial(uint32_t index, bool excludeLetter) {
    /*! With FT_LIST_BY_INDEX, FT_ListDevices wants the index passed in place of a pointer */
    PVOID indexArgument = reinterpret_cast<PVOID>(static_cast<uintptr_t>(index));
    char buffer[64];

    FT_STATUS status = FTW_ListDevices(indexArgument, buffer, FT_LIST_BY_INDEX);
    if (status != FT_OK) {
        return "";
    }

    std::string serial = buffer;
    if (excludeLetter && !serial.empty()) {
        serial.pop_back(); /*!< Removes the channel letter (A, B, ...) */
    }
    return serial;
}

bool Ftd2xxWrapper::getDeviceCount(DWORD &numDevs) {
    numDevs = 0;

    /*! No FTDI device plugged in: no need to load FTD2XX at all */
    if (!FtdiLibraries::isFtd2xxLoaded() && !isFtdiDevicePresent()) {
        return true;
    }

    FT_STATUS status = FTW_ListDevices(&numDevs, nullptr, FT_LIST_NUMBER_ONLY);
    return status == FT_OK;
}
