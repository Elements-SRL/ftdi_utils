#ifndef FTDICONNECTIONMUTEX_H
#define FTDICONNECTIONMUTEX_H

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include "WinTypes.h"
#else
#endif
/*! The FTDI headers are included only for the types and the constants (FT_HANDLE, FT_STATUS,
 *  ChannelConfig, ...). The FTDI functions are loaded at runtime (see README.md), so neither
 *  ftd2xx nor libMPSSE has to be linked and the application starts even without their DLLs. */
#include "ftd2xx.h"
#include "libmpsse_spi.h"

/*! \brief Why the last loading of an FTDI library failed. */
enum class FtdiLoadError {
    None,               /*!< No error */
    LibraryNotFound,    /*!< The file could not be loaded (missing, wrong architecture, missing dependency) */
    SymbolMissing       /*!< The file was loaded but does not export all the required functions */
};

/*! \brief Thread-safe access to the FTDI libraries (FTD2XX and libMPSSE), loaded at runtime.
 *
 *  - Functions without a handle (list, open, close) are serialized by a global mutex.
 *  - Functions with a handle are serialized per handle: two different devices can be used
 *    in parallel, the same device by one thread at a time.
 *  - The libraries are loaded by the first call that needs them. If they cannot be loaded the
 *    functions return FT_OTHER_ERROR (or nullptr / false), see getLastLoadError().
 */
class Ftd2xxWrapper {
public:
    /**************************\
     *  Libraries management  *
    \**************************/

    /*! \brief Loads FTD2XX now instead of at the first call that needs it.
     *  \return true if FTD2XX is loaded. */
    static bool loadFtd2xx();

    /*! \brief Loads libMPSSE (and FTD2XX first) now instead of at the first SPI call.
     *  \return true if both are loaded. */
    static bool loadMpsse();

    static bool isFtd2xxLoaded();
    static bool isMpsseLoaded();

    /*! \brief Unloads both libraries.
     *  \return false (and does nothing) while any FTDI handle is still open. */
    static bool unloadLibraries();

    /*! \brief Description of the last loading failure, empty if the last loading succeeded. */
    static std::string getLastLoadError();

    /*! \brief Type of the last loading failure. */
    static FtdiLoadError getLastLoadErrorType();

    /*! \brief Folder containing the FTDI libraries.
     *  Empty (default): folder of the executable, then System32. The PATH is never searched.
     *  Set: only this folder. Takes effect at the next load. */
    static void setLibraryDirectory(const std::filesystem::path &directory);

    /***************************\
     *  Device presence check  *
    \***************************/

    /*! \brief Tells whether a USB device with one of the configured vendor ids is plugged in.
     *  It does not use FTD2XX (Windows: Configuration Manager, Linux: sysfs), so it can be
     *  called without loading any FTDI library.
     *  \return true also when the check cannot be performed, so that the enumeration
     *  falls back to FTD2XX. */
    static bool isFtdiDevicePresent();

    /*! \brief Vendor ids used by isFtdiDevicePresent(). Default {0x0403} (FTDI).
     *  An empty list disables the check: isFtdiDevicePresent() always returns true. */
    static void setUsbVendorIds(const std::vector<uint16_t> &vendorIds);

    /************************************\
     *  Global operations (no handle)  *
    \************************************/

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

    /*****************************\
     *  Handle-based operations  *
    \*****************************/

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

    /*! \brief Forgets a handle (its mutex and its open state). Called by the close functions. */
    static void CleanupHandle(FT_HANDLE handle);

    /****************************\
     *  Higher level utilities  *
    \****************************/

    static int32_t getDeviceIndex(std::string serial);
    static std::string getDeviceSerial(uint32_t index, bool excludeLetter);
    static bool getDeviceCount(DWORD &numDevs);
};

#endif // FTDICONNECTIONMUTEX_H
