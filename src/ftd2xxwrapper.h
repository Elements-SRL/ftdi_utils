#ifndef FTDICONNECTIONMUTEX_H
#define FTDICONNECTIONMUTEX_H

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include "WinTypes.h"
#else
#endif
/*! Vendor headers are included only for types and constants (FT_HANDLE, FT_STATUS, ChannelConfig, ...).
 *  The functions are resolved at runtime with LoadLibrary/dlopen, so neither ftd2xx nor libMPSSE
 *  must be linked: the application starts even when the DLLs are missing. */
#include "ftd2xx.h"
#include "libmpsse_spi.h"

class Ftd2xxWrapper {
public:
    /*! Dynamic loading
     *  The libraries are loaded lazily by the first call that needs them:
     *  - FTW_* functions load ftd2xx
     *  - SPIW_* functions and FTW_WriteGPIO/FTW_ReadGPIO load libMPSSE (and ftd2xx first)
     *  getDeviceCount() first checks whether a USB device with an FTDI vendor id is plugged in
     *  (without using ftd2xx) and loads nothing when there is none.
     *  If a library cannot be loaded the wrappers return FT_OTHER_ERROR (or nullptr / false). */
    static bool loadFtd2xx();
    static bool loadMpsse();
    static bool isFtd2xxLoaded();
    static bool isMpsseLoaded();
    /*! Unloads both libraries. Fails (returns false) while any handle is still open. */
    static bool unloadLibraries();
    /*! Human-readable description of the last load failure (empty if none). */
    static std::string getLastLoadError();
    /*! Optional: folder (UTF-8) containing the DLLs. Empty (default) = application folder and
     *  system folders. Takes effect at the next load. */
    static void setLibraryDirectory(const std::string &directory);

    /*! Device presence check that does NOT use ftd2xx (Windows: Configuration Manager, Linux: sysfs).
     *  Returns true when a USB device with one of the configured vendor ids is present.
     *  If the check cannot be performed it returns true, so that enumeration falls back to ftd2xx. */
    static bool isFtdiDevicePresent();
    /*! Vendor ids considered by isFtdiDevicePresent(). Default {0x0403} (FTDI).
     *  An empty list disables the check (isFtdiDevicePresent() always returns true). */
    static void setUsbVendorIds(const std::vector<uint16_t> &vendorIds);

    /*! Global operations (no handle) */
    static FT_STATUS FTW_ListDevices(PVOID pArg1, PVOID pArg2, DWORD Flags);
    static FT_STATUS FTW_CreateDeviceInfoList(LPDWORD numDevs);
    static FT_STATUS FTW_GetDeviceInfoList(FT_DEVICE_LIST_INFO_NODE* list, LPDWORD numDevs);
    static FT_STATUS FTW_Open(int deviceNumber, FT_HANDLE* handle);
    static FT_STATUS FTW_OpenEx(PVOID pArg1, DWORD flags, FT_HANDLE* handle);
    static FT_STATUS FTW_Close(FT_HANDLE handle);

    static void SPIW_Init_libMPSSE();
    static void SPIW_Cleanup_libMPSSE();
    static FT_STATUS SPIW_OpenChannel(DWORD index, FT_HANDLE *handle);
    static FT_HANDLE SPIW_OpenChannelBySerial(std::string serial);
    static FT_STATUS SPIW_CloseChannel(FT_HANDLE handle);

    /*! Handle-based operations */
    static FT_STATUS FTW_SetBitMode(FT_HANDLE ftHandle, UCHAR ucMask, UCHAR ucEnable);
    static FT_STATUS FTW_SetLatencyTimer( FT_HANDLE ftHandle, UCHAR ucLatency);
    static FT_STATUS FTW_SetUSBParameters( FT_HANDLE ftHandle, ULONG ulInTransferSize, ULONG ulOutTransferSize);
    static FT_STATUS FTW_SetFlowControl( FT_HANDLE ftHandle, USHORT FlowControl, UCHAR XonChar, UCHAR XoffChar);
    static FT_STATUS FTW_GetQueueStatus(FT_HANDLE ftHandle, DWORD *dwRxBytes);
    static FT_STATUS FTW_Write(FT_HANDLE handle, LPVOID buffer, DWORD bytesToWrite, LPDWORD bytesWritten);
    static FT_STATUS FTW_Read(FT_HANDLE handle, LPVOID buffer, DWORD bytesToRead, LPDWORD bytesRead);
    static FT_STATUS FTW_Purge(FT_HANDLE handle, ULONG mask);
    static FT_STATUS FTW_ResetDevice(FT_HANDLE handle);
    static FT_STATUS FTW_ReadEE(FT_HANDLE handle, DWORD wordOffset, LPWORD value);

    static FT_STATUS SPIW_InitChannel(FT_HANDLE handle, ChannelConfig *config);
    static FT_STATUS SPIW_Read(FT_HANDLE handle, UCHAR *buffer,
                               DWORD sizeToTransfer, LPDWORD sizeTransfered, DWORD options);
    static FT_STATUS SPIW_Write(FT_HANDLE handle, UCHAR *buffer,
                                DWORD sizeToTransfer, LPDWORD sizeTransfered, DWORD options);
    static FT_STATUS FTW_WriteGPIO(FT_HANDLE handle, UCHAR dir, UCHAR value);
    static FT_STATUS FTW_ReadGPIO(FT_HANDLE handle, UCHAR *value);

    static void CleanupHandle(FT_HANDLE handle);

    /*! Higher level utilities */
    static int32_t getDeviceIndex(std::string serial);
    static std::string getDeviceSerial(uint32_t index, bool excludeLetter);
    static bool getDeviceCount(DWORD &numDevs);

private:
    static std::mutex globalMutex;
    static std::mutex mapMutex;
    static std::map<FT_HANDLE, std::shared_ptr<std::mutex>> handleMutexMap;
    static std::set<FT_HANDLE> openHandles;

    static std::shared_ptr<std::mutex> getHandleMutex(FT_HANDLE handle);
    static void registerHandle(FT_HANDLE handle);
};

#endif // FTDICONNECTIONMUTEX_H
