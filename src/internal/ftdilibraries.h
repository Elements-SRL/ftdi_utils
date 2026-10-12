#ifndef FTDILIBRARIES_H
#define FTDILIBRARIES_H

#include <filesystem>
#include <string>

#include "ftd2xxwrapper.h" /*!< FTDI headers (types and declarations) and FtdiLoadError */

/*! \brief Pointers to the FTD2XX functions used by ftdi_utils.
 *
 *  The type of each pointer is taken from the FTDI header with decltype: if FTDI changes a
 *  parameter or the calling convention, the pointer type changes with it. A hand-written type
 *  could silently go out of sync and crash at runtime.
 *  decltype does not use the function, so the linker does not need ftd2xx.lib.
 */
struct Ftd2xxFunctions {
    decltype(::FT_ListDevices) *ListDevices = nullptr;
    decltype(::FT_CreateDeviceInfoList) *CreateDeviceInfoList = nullptr;
    decltype(::FT_GetDeviceInfoList) *GetDeviceInfoList = nullptr;
    decltype(::FT_Open) *Open = nullptr;
    decltype(::FT_OpenEx) *OpenEx = nullptr;
    decltype(::FT_Close) *Close = nullptr;
    decltype(::FT_SetBitMode) *SetBitMode = nullptr;
    decltype(::FT_SetLatencyTimer) *SetLatencyTimer = nullptr;
    decltype(::FT_SetUSBParameters) *SetUSBParameters = nullptr;
    decltype(::FT_SetFlowControl) *SetFlowControl = nullptr;
    decltype(::FT_GetQueueStatus) *GetQueueStatus = nullptr;
    decltype(::FT_Write) *Write = nullptr;
    decltype(::FT_Read) *Read = nullptr;
    decltype(::FT_Purge) *Purge = nullptr;
    decltype(::FT_ResetDevice) *ResetDevice = nullptr;
    decltype(::FT_ReadEE) *ReadEE = nullptr;
};

/*! \brief Pointers to the libMPSSE functions used by ftdi_utils (see Ftd2xxFunctions). */
struct MpsseFunctions {
    decltype(::Init_libMPSSE) *Init = nullptr;       /*!< Optional: not exported by every libMPSSE version */
    decltype(::Cleanup_libMPSSE) *Cleanup = nullptr; /*!< Optional: not exported by every libMPSSE version */
    decltype(::SPI_GetNumChannels) *GetNumChannels = nullptr;
    decltype(::SPI_GetChannelInfo) *GetChannelInfo = nullptr;
    decltype(::SPI_OpenChannel) *OpenChannel = nullptr;
    decltype(::SPI_InitChannel) *InitChannel = nullptr;
    decltype(::SPI_CloseChannel) *CloseChannel = nullptr;
    decltype(::SPI_Read) *Read = nullptr;
    decltype(::SPI_Write) *Write = nullptr;
    decltype(::FT_WriteGPIO) *WriteGPIO = nullptr;
    decltype(::FT_ReadGPIO) *ReadGPIO = nullptr;
};

/*! \brief Runtime loading of FTD2XX and libMPSSE.
 *
 *  All the methods are static and thread-safe. The libraries are loaded at most once:
 *  further load calls return immediately while they stay loaded.
 *
 *  \note Internal to ftdi_utils: use Ftd2xxWrapper instead.
 */
class FtdiLibraries {
public:
    /*! \brief Loads FTD2XX. Returns true if it is loaded (now or before). */
    static bool loadFtd2xx();

    /*! \brief Loads libMPSSE, loading FTD2XX first. Returns true if both are loaded. */
    static bool loadMpsse();

    /*! \brief Unloads both libraries.
     *  \warning The caller must make sure that no function pointer is in use or will be used
     *  during the call (Ftd2xxWrapper refuses to unload while handles are open). */
    static void unload();

    static bool isFtd2xxLoaded();
    static bool isMpsseLoaded();

    /*! \brief The FTD2XX functions. Valid only while isFtd2xxLoaded() is true. */
    static const Ftd2xxFunctions &ftd2xx();

    /*! \brief The libMPSSE functions. Valid only while isMpsseLoaded() is true. */
    static const MpsseFunctions &mpsse();

    /*! \brief Folder containing the libraries. Empty = default Windows search
     *  (folder of the executable, then System32). Used by the next load. */
    static void setLibraryDirectory(const std::filesystem::path &directory);

    static FtdiLoadError lastErrorType();
    static std::string lastErrorMessage();
};

#endif // FTDILIBRARIES_H
