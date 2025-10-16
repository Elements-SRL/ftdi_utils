#include "ftd2xxwrapper.h"

std::mutex Ftd2xxWrapper::globalMutex;
std::mutex Ftd2xxWrapper::mapMutex;
std::map<FT_HANDLE, std::shared_ptr<std::mutex>> Ftd2xxWrapper::handleMutexMap;

// ============================================================================
// Internal helpers
// ============================================================================

std::shared_ptr<std::mutex> Ftd2xxWrapper::getHandleMutex(FT_HANDLE handle) {
    std::lock_guard<std::mutex> guard(mapMutex);
    auto &m = handleMutexMap[handle];
    if (!m) m = std::make_shared<std::mutex>();
    return m;
}

// ============================================================================
// Global ops
// ============================================================================

FT_STATUS Ftd2xxWrapper::FTW_ListDevices(PVOID pArg1, PVOID pArg2, DWORD Flags) {
    std::lock_guard<std::mutex> lock(globalMutex);
    return FT_ListDevices(pArg1, pArg2, Flags);
}

FT_STATUS Ftd2xxWrapper::FTW_CreateDeviceInfoList(LPDWORD numDevs) {
    std::lock_guard<std::mutex> lock(globalMutex);
    return FT_CreateDeviceInfoList(numDevs);
}

FT_STATUS Ftd2xxWrapper::FTW_GetDeviceInfoList(FT_DEVICE_LIST_INFO_NODE* list, LPDWORD numDevs) {
    std::lock_guard<std::mutex> lock(globalMutex);
    return FT_GetDeviceInfoList(list, numDevs);
}

FT_STATUS Ftd2xxWrapper::FTW_Open(int deviceNumber, FT_HANDLE* handle) {
    std::lock_guard<std::mutex> lock(globalMutex);
    return FT_Open(deviceNumber, handle);
}

FT_STATUS Ftd2xxWrapper::FTW_OpenEx(PVOID pArg1, DWORD flags, FT_HANDLE* handle) {
    std::lock_guard<std::mutex> lock(globalMutex);
    return FT_OpenEx(pArg1, flags, handle);
}

FT_STATUS Ftd2xxWrapper::FTW_Close(FT_HANDLE handle) {
    std::lock_guard<std::mutex> lock(globalMutex);
    auto status = FT_Close(handle);
    if (status == FT_OK)
        CleanupHandle(handle);
    return status;
}

void Ftd2xxWrapper::SPIW_Init_libMPSSE() {
    std::lock_guard<std::mutex> lock(globalMutex);
    Init_libMPSSE();
}

void Ftd2xxWrapper::SPIW_Cleanup_libMPSSE() {
    std::lock_guard<std::mutex> lock(globalMutex);
    Cleanup_libMPSSE();
}

FT_STATUS Ftd2xxWrapper::SPIW_OpenChannel(DWORD index, FT_HANDLE *handle) {
    std::lock_guard<std::mutex> lock(globalMutex);
    return SPI_OpenChannel(index, handle);
}

FT_STATUS Ftd2xxWrapper::SPIW_CloseChannel(FT_HANDLE handle) {
    std::lock_guard<std::mutex> lock(globalMutex);
    auto status = SPI_CloseChannel(handle);
    if (status == FT_OK)
        CleanupHandle(handle);
    return status;
}

// ============================================================================
// Handle ops
// ============================================================================

FT_STATUS Ftd2xxWrapper::FTW_SetBitMode(FT_HANDLE handle, UCHAR ucMask, UCHAR ucEnable) {
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return FT_SetBitMode(handle, ucMask, ucEnable);
}

FT_STATUS Ftd2xxWrapper::FTW_SetLatencyTimer(FT_HANDLE handle, UCHAR ucLatency) {
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return FT_SetLatencyTimer(handle, ucLatency);
}

FT_STATUS Ftd2xxWrapper::FTW_SetUSBParameters(FT_HANDLE handle, ULONG ulInTransferSize, ULONG ulOutTransferSize) {
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return FT_SetUSBParameters(handle, ulInTransferSize, ulOutTransferSize);
}

FT_STATUS Ftd2xxWrapper::FTW_SetFlowControl(FT_HANDLE handle, USHORT FlowControl, UCHAR XonChar, UCHAR XoffChar) {
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return FT_SetFlowControl(handle, FlowControl, XonChar, XoffChar);
}

FT_STATUS Ftd2xxWrapper::FTW_GetQueueStatus(FT_HANDLE handle, DWORD *dwRxBytes) {
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return FT_GetQueueStatus(handle, dwRxBytes);
}

FT_STATUS Ftd2xxWrapper::FTW_Write(FT_HANDLE handle, LPVOID buffer, DWORD bytesToWrite, LPDWORD bytesWritten) {
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return FT_Write(handle, buffer, bytesToWrite, bytesWritten);
}

FT_STATUS Ftd2xxWrapper::FTW_Read(FT_HANDLE handle, LPVOID buffer, DWORD bytesToRead, LPDWORD bytesRead) {
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return FT_Read(handle, buffer, bytesToRead, bytesRead);
}

FT_STATUS Ftd2xxWrapper::FTW_Purge(FT_HANDLE handle, ULONG mask) {
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return FT_Purge(handle, mask);
}

FT_STATUS Ftd2xxWrapper::FTW_ResetDevice(FT_HANDLE handle) {
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return FT_ResetDevice(handle);
}

FT_STATUS Ftd2xxWrapper::SPIW_InitChannel(FT_HANDLE handle, ChannelConfig *config) {
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return SPI_InitChannel(handle, config);
}

FT_STATUS Ftd2xxWrapper::SPIW_Read(FT_HANDLE handle, UCHAR *buffer,
                                   DWORD sizeToTransfer, LPDWORD sizeTransfered, DWORD options) {
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return SPI_Read(handle, buffer, sizeToTransfer, sizeTransfered, options);
}

FT_STATUS Ftd2xxWrapper::SPIW_Write(FT_HANDLE handle, UCHAR *buffer,
                                    DWORD sizeToTransfer, LPDWORD sizeTransfered, DWORD options) {
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return SPI_Write(handle, buffer, sizeToTransfer, sizeTransfered, options);
}

FT_STATUS Ftd2xxWrapper::FTW_WriteGPIO(FT_HANDLE handle, UCHAR dir, UCHAR value) {
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return FT_WriteGPIO(handle, dir, value);
}

FT_STATUS Ftd2xxWrapper::FTW_ReadGPIO(FT_HANDLE handle, UCHAR *value) {
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return FT_ReadGPIO(handle, value);
}

void Ftd2xxWrapper::CleanupHandle(FT_HANDLE handle) {
    std::lock_guard<std::mutex> guard(mapMutex);
    handleMutexMap.erase(handle);
}

int32_t Ftd2xxWrapper::getDeviceIndex(std::string serial) {
    /*! Gets number of devices */
    DWORD numDevs;
    bool devCountOk = getDeviceCount(numDevs);
    if (!devCountOk) {
        return -1;
    }
    else if (numDevs == 0) {
        return -1;
    }

    for (int32_t index = 0; index < numDevs; index++) {
        std::string deviceId = getDeviceSerial(index, false);
        if (deviceId == serial) {
            return index;
        }
    }
    return -1;
}

std::string Ftd2xxWrapper::getDeviceSerial(uint32_t index, bool excludeLetter) {
    char buffer[64];
    std::string serial;
    FT_STATUS FT_Result = Ftd2xxWrapper::FTW_ListDevices((PVOID)index, buffer, FT_LIST_BY_INDEX);
    if (FT_Result == FT_OK) {
        serial = buffer;
        if (excludeLetter) {
            return serial.substr(0, serial.size()-1); /*!< Removes channel character */
        }
        else {
            return serial;
        }
    }
    return "";
}

bool Ftd2xxWrapper::getDeviceCount(DWORD &numDevs) {
    /*! Get the number of connected devices */
    numDevs = 0;
    FT_STATUS FT_Result = Ftd2xxWrapper::FTW_ListDevices(&numDevs, nullptr, FT_LIST_NUMBER_ONLY);
    if (FT_Result == FT_OK) {
        return true;
    }
    return false;
}
