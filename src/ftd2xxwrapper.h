#ifndef FTDICONNECTIONMUTEX_H
#define FTDICONNECTIONMUTEX_H

#include <map>
#include <memory>
#include <mutex>
#include <windows.h>
#include "ftd2xx.h"
#include "libmpsse_spi.h"

class Ftd2xxWrapper {
public:
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

    static std::shared_ptr<std::mutex> getHandleMutex(FT_HANDLE handle);
};

#endif // FTDICONNECTIONMUTEX_H
