#include "ftd2xxwrapper.h"

#include <atomic>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
#include <cfgmgr32.h>
#include <cwchar>
#include <cwctype>
#ifdef _MSC_VER
#pragma comment(lib, "cfgmgr32.lib")
#endif
#ifndef CM_GETIDLIST_FILTER_PRESENT
#define CM_GETIDLIST_FILTER_PRESENT (0x00000100)
#endif
#else
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#endif

/*! Library file names. Can be overridden at build time (see CMakeLists.txt). */
#ifndef FTDI_UTILS_FTD2XX_LIBNAME
#ifdef _WIN32
#define FTDI_UTILS_FTD2XX_LIBNAME "FTD2XX.dll"
#else
#define FTDI_UTILS_FTD2XX_LIBNAME "libftd2xx.so"
#endif
#endif

#ifndef FTDI_UTILS_MPSSE_LIBNAME
#ifdef _WIN32
#ifdef _DEBUG
#define FTDI_UTILS_MPSSE_LIBNAME "MPSSEd.dll"
#else
#define FTDI_UTILS_MPSSE_LIBNAME "MPSSE.dll"
#endif
#else
#define FTDI_UTILS_MPSSE_LIBNAME "libmpsse.so"
#endif
#endif

std::mutex Ftd2xxWrapper::globalMutex;
std::mutex Ftd2xxWrapper::mapMutex;
std::map<FT_HANDLE, std::shared_ptr<std::mutex>> Ftd2xxWrapper::handleMutexMap;
std::set<FT_HANDLE> Ftd2xxWrapper::openHandles;

// ============================================================================
// Dynamic loader
// ============================================================================

namespace {

#ifdef _WIN32
using LibHandle = HMODULE;
#else
using LibHandle = void *;
#endif

/*! Function pointer types are taken from the vendor headers with decltype, so they always
 *  match the declarations (including the calling convention). decltype is an unevaluated
 *  context: no reference to the import library is generated. */
struct Ftd2xxApi {
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

struct MpsseApi {
    decltype(::Init_libMPSSE) *Init = nullptr;       /*!< optional */
    decltype(::Cleanup_libMPSSE) *Cleanup = nullptr; /*!< optional */
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

std::mutex loadMutex; /*!< protects everything below; always taken after globalMutex */
LibHandle ftd2xxLib = nullptr;
LibHandle mpsseLib = nullptr;
Ftd2xxApi ft;
MpsseApi mp;
std::atomic<bool> ftLoaded{false};
std::atomic<bool> mpLoaded{false};
std::string lastLoadError;
std::string libraryDirectory;
std::vector<uint16_t> usbVendorIds{0x0403};

constexpr FT_STATUS libraryNotLoadedStatus = FT_OTHER_ERROR;

#ifdef _WIN32
std::wstring toWide(const std::string &s) {
    if (s.empty()) return std::wstring();
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), len);
    return w;
}

std::string systemErrorString(DWORD err) {
    char *msg = nullptr;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, err, 0, (LPSTR)&msg, 0, nullptr);
    std::string s = msg ? msg : "";
    if (msg) LocalFree(msg);
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) s.pop_back();
    return "error " + std::to_string(err) + (s.empty() ? "" : " (" + s + ")");
}
#endif

LibHandle openLibrary(const char *name, std::string &error) {
#ifdef _WIN32
    LibHandle lib = nullptr;
    if (!libraryDirectory.empty()) {
        std::wstring path = toWide(libraryDirectory);
        if (path.back() != L'\\' && path.back() != L'/') path += L'\\';
        path += toWide(name);
        /*! Dependencies (MPSSE.dll -> FTD2XX.dll) are searched in the same folder too */
        lib = LoadLibraryExW(path.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    } else {
        /*! Application folder + System32 only: no current directory, no PATH (avoids DLL planting) */
        lib = LoadLibraryExW(toWide(name).c_str(), nullptr, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        if (lib == nullptr) {
            /*! Fallback to the standard search order (e.g. DLLs on PATH in a development setup) */
            lib = LoadLibraryW(toWide(name).c_str());
        }
    }
    if (lib == nullptr) {
        error = std::string("cannot load ") + name + ": " + systemErrorString(GetLastError());
    }
    return lib;
#else
    std::string path = name;
    if (!libraryDirectory.empty()) {
        path = libraryDirectory;
        if (path.back() != '/') path += '/';
        path += name;
    }
    /*! RTLD_GLOBAL: libmpsse resolves the FT_* symbols from the already loaded libftd2xx */
    LibHandle lib = dlopen(path.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (lib == nullptr) {
        const char *e = dlerror();
        error = std::string("cannot load ") + name + ": " + (e ? e : "unknown error");
    }
    return lib;
#endif
}

void closeLibrary(LibHandle lib) {
    if (lib == nullptr) return;
#ifdef _WIN32
    FreeLibrary(lib);
#else
    dlclose(lib);
#endif
}

void *findSymbol(LibHandle lib, const char *name) {
#ifdef _WIN32
    return reinterpret_cast<void *>(GetProcAddress(lib, name));
#else
    return dlsym(lib, name);
#endif
}

template <typename Fn>
void resolve(LibHandle lib, const char *name, Fn *&fn, std::string &missing) {
    fn = reinterpret_cast<Fn *>(findSymbol(lib, name));
    if (fn == nullptr) {
        missing += missing.empty() ? name : std::string(", ") + name;
    }
}

template <typename Fn>
void resolveOptional(LibHandle lib, const char *name, Fn *&fn) {
    fn = reinterpret_cast<Fn *>(findSymbol(lib, name));
}

/*! Must be called with loadMutex held */
bool loadFtd2xxLocked() {
    if (ftLoaded.load()) return true;

    std::string error;
    LibHandle lib = openLibrary(FTDI_UTILS_FTD2XX_LIBNAME, error);
    if (lib == nullptr) {
        lastLoadError = error;
        return false;
    }

    Ftd2xxApi api;
    std::string missing;
    resolve(lib, "FT_ListDevices", api.ListDevices, missing);
    resolve(lib, "FT_CreateDeviceInfoList", api.CreateDeviceInfoList, missing);
    resolve(lib, "FT_GetDeviceInfoList", api.GetDeviceInfoList, missing);
    resolve(lib, "FT_Open", api.Open, missing);
    resolve(lib, "FT_OpenEx", api.OpenEx, missing);
    resolve(lib, "FT_Close", api.Close, missing);
    resolve(lib, "FT_SetBitMode", api.SetBitMode, missing);
    resolve(lib, "FT_SetLatencyTimer", api.SetLatencyTimer, missing);
    resolve(lib, "FT_SetUSBParameters", api.SetUSBParameters, missing);
    resolve(lib, "FT_SetFlowControl", api.SetFlowControl, missing);
    resolve(lib, "FT_GetQueueStatus", api.GetQueueStatus, missing);
    resolve(lib, "FT_Write", api.Write, missing);
    resolve(lib, "FT_Read", api.Read, missing);
    resolve(lib, "FT_Purge", api.Purge, missing);
    resolve(lib, "FT_ResetDevice", api.ResetDevice, missing);
    resolve(lib, "FT_ReadEE", api.ReadEE, missing);

    if (!missing.empty()) {
        lastLoadError = std::string(FTDI_UTILS_FTD2XX_LIBNAME) + ": missing symbols " + missing;
        closeLibrary(lib);
        return false;
    }

    ftd2xxLib = lib;
    ft = api;
    lastLoadError.clear();
    ftLoaded.store(true);
    return true;
}

/*! Must be called with loadMutex held */
bool loadMpsseLocked() {
    if (mpLoaded.load()) return true;
    /*! libMPSSE depends on ftd2xx: load it first so that it is resolved from the same place */
    if (!loadFtd2xxLocked()) return false;

    std::string error;
    LibHandle lib = openLibrary(FTDI_UTILS_MPSSE_LIBNAME, error);
    if (lib == nullptr) {
        lastLoadError = error;
        return false;
    }

    MpsseApi api;
    std::string missing;
    resolveOptional(lib, "Init_libMPSSE", api.Init);
    resolveOptional(lib, "Cleanup_libMPSSE", api.Cleanup);
    resolve(lib, "SPI_GetNumChannels", api.GetNumChannels, missing);
    resolve(lib, "SPI_GetChannelInfo", api.GetChannelInfo, missing);
    resolve(lib, "SPI_OpenChannel", api.OpenChannel, missing);
    resolve(lib, "SPI_InitChannel", api.InitChannel, missing);
    resolve(lib, "SPI_CloseChannel", api.CloseChannel, missing);
    resolve(lib, "SPI_Read", api.Read, missing);
    resolve(lib, "SPI_Write", api.Write, missing);
    resolve(lib, "FT_WriteGPIO", api.WriteGPIO, missing);
    resolve(lib, "FT_ReadGPIO", api.ReadGPIO, missing);

    if (!missing.empty()) {
        lastLoadError = std::string(FTDI_UTILS_MPSSE_LIBNAME) + ": missing symbols " + missing;
        closeLibrary(lib);
        return false;
    }

    mpsseLib = lib;
    mp = api;
    lastLoadError.clear();
    mpLoaded.store(true);
    return true;
}

bool ensureFtd2xx() {
    if (ftLoaded.load(std::memory_order_acquire)) return true;
    std::lock_guard<std::mutex> lock(loadMutex);
    return loadFtd2xxLocked();
}

bool ensureMpsse() {
    if (mpLoaded.load(std::memory_order_acquire)) return true;
    std::lock_guard<std::mutex> lock(loadMutex);
    return loadMpsseLocked();
}

} // namespace

bool Ftd2xxWrapper::loadFtd2xx() {
    return ensureFtd2xx();
}

bool Ftd2xxWrapper::loadMpsse() {
    return ensureMpsse();
}

bool Ftd2xxWrapper::isFtd2xxLoaded() {
    return ftLoaded.load();
}

bool Ftd2xxWrapper::isMpsseLoaded() {
    return mpLoaded.load();
}

bool Ftd2xxWrapper::unloadLibraries() {
    std::lock_guard<std::mutex> lock(globalMutex);
    {
        std::lock_guard<std::mutex> guard(mapMutex);
        if (!openHandles.empty()) {
            return false;
        }
        handleMutexMap.clear();
    }
    std::lock_guard<std::mutex> loadLock(loadMutex);
    mpLoaded.store(false);
    ftLoaded.store(false);
    closeLibrary(mpsseLib); /*!< libMPSSE first: it depends on ftd2xx */
    closeLibrary(ftd2xxLib);
    mpsseLib = nullptr;
    ftd2xxLib = nullptr;
    mp = MpsseApi();
    ft = Ftd2xxApi();
    return true;
}

std::string Ftd2xxWrapper::getLastLoadError() {
    std::lock_guard<std::mutex> lock(loadMutex);
    return lastLoadError;
}

void Ftd2xxWrapper::setLibraryDirectory(const std::string &directory) {
    std::lock_guard<std::mutex> lock(loadMutex);
    libraryDirectory = directory;
}

void Ftd2xxWrapper::setUsbVendorIds(const std::vector<uint16_t> &vendorIds) {
    std::lock_guard<std::mutex> lock(loadMutex);
    usbVendorIds = vendorIds;
}

bool Ftd2xxWrapper::isFtdiDevicePresent() {
    std::vector<uint16_t> vids;
    {
        std::lock_guard<std::mutex> lock(loadMutex);
        vids = usbVendorIds;
    }
    if (vids.empty()) {
        return true;
    }

#ifdef _WIN32
    /*! Instance ids look like USB\VID_0403&PID_6010\FT1A2B3C */
    const ULONG flags = CM_GETIDLIST_FILTER_ENUMERATOR | CM_GETIDLIST_FILTER_PRESENT;
    std::vector<wchar_t> buffer;
    CONFIGRET cr;
    do {
        ULONG len = 0;
        cr = CM_Get_Device_ID_List_SizeW(&len, L"USB", flags);
        if (cr != CR_SUCCESS) {
            return true; /*!< cannot tell: let ftd2xx decide */
        }
        buffer.assign(len + 1, L'\0');
        cr = CM_Get_Device_ID_ListW(L"USB", buffer.data(), len, flags);
    } while (cr == CR_BUFFER_SMALL);
    if (cr != CR_SUCCESS) {
        return true;
    }

    for (const wchar_t *id = buffer.data(); *id != L'\0'; id += wcslen(id) + 1) {
        std::wstring upper(id);
        for (auto &c : upper) c = (wchar_t)towupper(c);
        for (uint16_t vid : vids) {
            wchar_t pattern[16];
            swprintf(pattern, 16, L"VID_%04X", vid);
            if (upper.find(pattern) != std::wstring::npos) {
                return true;
            }
        }
    }
    return false;
#elif __linux__
    std::error_code ec;
    const std::filesystem::path root("/sys/bus/usb/devices");
    if (!std::filesystem::exists(root, ec)) {
        return true;
    }
    for (const auto &entry : std::filesystem::directory_iterator(root, ec)) {
        std::ifstream f(entry.path() / "idVendor");
        unsigned int vid = 0;
        if (f >> std::hex >> vid) {
            for (uint16_t v : vids) {
                if (vid == v) {
                    return true;
                }
            }
        }
    }
    return false;
#else
    return true;
#endif
}

// ============================================================================
// Internal helpers
// ============================================================================

std::shared_ptr<std::mutex> Ftd2xxWrapper::getHandleMutex(FT_HANDLE handle) {
    std::lock_guard<std::mutex> guard(mapMutex);
    auto &m = handleMutexMap[handle];
    if (!m) m = std::make_shared<std::mutex>();
    return m;
}

void Ftd2xxWrapper::registerHandle(FT_HANDLE handle) {
    std::lock_guard<std::mutex> guard(mapMutex);
    openHandles.insert(handle);
}

// ============================================================================
// Global ops
// ============================================================================

FT_STATUS Ftd2xxWrapper::FTW_ListDevices(PVOID pArg1, PVOID pArg2, DWORD Flags) {
    std::lock_guard<std::mutex> lock(globalMutex);
    if (!ensureFtd2xx()) return libraryNotLoadedStatus;
    return ft.ListDevices(pArg1, pArg2, Flags);
}

FT_STATUS Ftd2xxWrapper::FTW_CreateDeviceInfoList(LPDWORD numDevs) {
    std::lock_guard<std::mutex> lock(globalMutex);
    if (!ensureFtd2xx()) return libraryNotLoadedStatus;
    return ft.CreateDeviceInfoList(numDevs);
}

FT_STATUS Ftd2xxWrapper::FTW_GetDeviceInfoList(FT_DEVICE_LIST_INFO_NODE* list, LPDWORD numDevs) {
    std::lock_guard<std::mutex> lock(globalMutex);
    if (!ensureFtd2xx()) return libraryNotLoadedStatus;
    return ft.GetDeviceInfoList(list, numDevs);
}

FT_STATUS Ftd2xxWrapper::FTW_Open(int deviceNumber, FT_HANDLE* handle) {
    std::lock_guard<std::mutex> lock(globalMutex);
    if (!ensureFtd2xx()) return libraryNotLoadedStatus;
    auto status = ft.Open(deviceNumber, handle);
    if (status == FT_OK)
        registerHandle(*handle);
    return status;
}

FT_STATUS Ftd2xxWrapper::FTW_OpenEx(PVOID pArg1, DWORD flags, FT_HANDLE* handle) {
    std::lock_guard<std::mutex> lock(globalMutex);
    if (!ensureFtd2xx()) return libraryNotLoadedStatus;
    auto status = ft.OpenEx(pArg1, flags, handle);
    if (status == FT_OK)
        registerHandle(*handle);
    return status;
}

FT_STATUS Ftd2xxWrapper::FTW_Close(FT_HANDLE handle) {
    std::lock_guard<std::mutex> lock(globalMutex);
    if (!ftLoaded.load()) return libraryNotLoadedStatus;
    auto status = ft.Close(handle);
    if (status == FT_OK)
        CleanupHandle(handle);
    return status;
}

void Ftd2xxWrapper::SPIW_Init_libMPSSE() {
    std::lock_guard<std::mutex> lock(globalMutex);
    if (!ensureMpsse()) return;
    if (mp.Init) mp.Init();
}

void Ftd2xxWrapper::SPIW_Cleanup_libMPSSE() {
    std::lock_guard<std::mutex> lock(globalMutex);
    if (!mpLoaded.load()) return;
    if (mp.Cleanup) mp.Cleanup();
}

FT_STATUS Ftd2xxWrapper::SPIW_OpenChannel(DWORD index, FT_HANDLE *handle) {
    std::lock_guard<std::mutex> lock(globalMutex);
    if (!ensureMpsse()) return libraryNotLoadedStatus;
    auto status = mp.OpenChannel(index, handle);
    if (status == FT_OK)
        registerHandle(*handle);
    return status;
}

FT_HANDLE Ftd2xxWrapper::SPIW_OpenChannelBySerial(std::string serial) {
    std::lock_guard<std::mutex> lock(globalMutex);
    if (!ensureMpsse()) return nullptr;
    DWORD numChannels = 0;
    FT_DEVICE_LIST_INFO_NODE chanInfo;
    FT_HANDLE handle = nullptr;
    FT_STATUS status;

    // 1. Get the current number of SPI channels
    status = mp.GetNumChannels(&numChannels);
    if (status != FT_OK || numChannels == 0) {
        return nullptr;
    }

    // 2. Iterate through each channel to find the match
    for (DWORD i = 0; i < numChannels; i++) {
        status = mp.GetChannelInfo(i, &chanInfo);
        if (status == FT_OK) {
            // Check if this channel's serial number matches your target
            if (strcmp(chanInfo.SerialNumber, serial.c_str()) == 0) {

                // 3. Open the channel using the index we JUST verified
                status = mp.OpenChannel(i, &handle);
                if (status == FT_OK) {
                    registerHandle(handle);
                    return handle;
                }
            }
        }
    }

    return nullptr;
}

FT_STATUS Ftd2xxWrapper::SPIW_CloseChannel(FT_HANDLE handle) {
    std::lock_guard<std::mutex> lock(globalMutex);
    if (!mpLoaded.load()) return libraryNotLoadedStatus;
    auto status = mp.CloseChannel(handle);
    if (status == FT_OK)
        CleanupHandle(handle);
    return status;
}

// ============================================================================
// Handle ops
// A valid handle implies that the library is loaded (unloadLibraries() refuses
// to unload while handles are open), the checks only protect against misuse.
// ============================================================================

FT_STATUS Ftd2xxWrapper::FTW_SetBitMode(FT_HANDLE handle, UCHAR ucMask, UCHAR ucEnable) {
    if (!ftLoaded.load()) return libraryNotLoadedStatus;
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return ft.SetBitMode(handle, ucMask, ucEnable);
}

FT_STATUS Ftd2xxWrapper::FTW_SetLatencyTimer(FT_HANDLE handle, UCHAR ucLatency) {
    if (!ftLoaded.load()) return libraryNotLoadedStatus;
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return ft.SetLatencyTimer(handle, ucLatency);
}

FT_STATUS Ftd2xxWrapper::FTW_SetUSBParameters(FT_HANDLE handle, ULONG ulInTransferSize, ULONG ulOutTransferSize) {
    if (!ftLoaded.load()) return libraryNotLoadedStatus;
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return ft.SetUSBParameters(handle, ulInTransferSize, ulOutTransferSize);
}

FT_STATUS Ftd2xxWrapper::FTW_SetFlowControl(FT_HANDLE handle, USHORT FlowControl, UCHAR XonChar, UCHAR XoffChar) {
    if (!ftLoaded.load()) return libraryNotLoadedStatus;
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return ft.SetFlowControl(handle, FlowControl, XonChar, XoffChar);
}

FT_STATUS Ftd2xxWrapper::FTW_GetQueueStatus(FT_HANDLE handle, DWORD *dwRxBytes) {
    if (!ftLoaded.load()) return libraryNotLoadedStatus;
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return ft.GetQueueStatus(handle, dwRxBytes);
}

FT_STATUS Ftd2xxWrapper::FTW_Write(FT_HANDLE handle, LPVOID buffer, DWORD bytesToWrite, LPDWORD bytesWritten) {
    if (!ftLoaded.load()) return libraryNotLoadedStatus;
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return ft.Write(handle, buffer, bytesToWrite, bytesWritten);
}

FT_STATUS Ftd2xxWrapper::FTW_Read(FT_HANDLE handle, LPVOID buffer, DWORD bytesToRead, LPDWORD bytesRead) {
    if (!ftLoaded.load()) return libraryNotLoadedStatus;
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return ft.Read(handle, buffer, bytesToRead, bytesRead);
}

FT_STATUS Ftd2xxWrapper::FTW_Purge(FT_HANDLE handle, ULONG mask) {
    if (!ftLoaded.load()) return libraryNotLoadedStatus;
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return ft.Purge(handle, mask);
}

FT_STATUS Ftd2xxWrapper::FTW_ResetDevice(FT_HANDLE handle) {
    if (!ftLoaded.load()) return libraryNotLoadedStatus;
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return ft.ResetDevice(handle);
}

FT_STATUS Ftd2xxWrapper::FTW_ReadEE(FT_HANDLE handle, DWORD wordOffset, LPWORD value) {
    if (!ftLoaded.load()) return libraryNotLoadedStatus;
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return ft.ReadEE(handle, wordOffset, value);
}

FT_STATUS Ftd2xxWrapper::SPIW_InitChannel(FT_HANDLE handle, ChannelConfig *config) {
    if (!mpLoaded.load()) return libraryNotLoadedStatus;
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return mp.InitChannel(handle, config);
}

FT_STATUS Ftd2xxWrapper::SPIW_Read(FT_HANDLE handle, UCHAR *buffer,
                                   DWORD sizeToTransfer, LPDWORD sizeTransfered, DWORD options) {
    if (!mpLoaded.load()) return libraryNotLoadedStatus;
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return mp.Read(handle, buffer, sizeToTransfer, sizeTransfered, options);
}

FT_STATUS Ftd2xxWrapper::SPIW_Write(FT_HANDLE handle, UCHAR *buffer,
                                    DWORD sizeToTransfer, LPDWORD sizeTransfered, DWORD options) {
    if (!mpLoaded.load()) return libraryNotLoadedStatus;
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return mp.Write(handle, buffer, sizeToTransfer, sizeTransfered, options);
}

FT_STATUS Ftd2xxWrapper::FTW_WriteGPIO(FT_HANDLE handle, UCHAR dir, UCHAR value) {
    if (!mpLoaded.load()) return libraryNotLoadedStatus;
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return mp.WriteGPIO(handle, dir, value);
}

FT_STATUS Ftd2xxWrapper::FTW_ReadGPIO(FT_HANDLE handle, UCHAR *value) {
    if (!mpLoaded.load()) return libraryNotLoadedStatus;
    auto m = getHandleMutex(handle);
    std::lock_guard<std::mutex> lock(*m);
    return mp.ReadGPIO(handle, value);
}

void Ftd2xxWrapper::CleanupHandle(FT_HANDLE handle) {
    std::lock_guard<std::mutex> guard(mapMutex);
    handleMutexMap.erase(handle);
    openHandles.erase(handle);
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

    for (int32_t index = 0; index < (int32_t)numDevs; index++) {
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
    FT_STATUS FT_Result = Ftd2xxWrapper::FTW_ListDevices((PVOID)(uintptr_t)index, buffer, FT_LIST_BY_INDEX);
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
    if (!isFtd2xxLoaded() && !isFtdiDevicePresent()) {
        /*! No FTDI device plugged in: do not even load ftd2xx */
        return true;
    }
    FT_STATUS FT_Result = Ftd2xxWrapper::FTW_ListDevices(&numDevs, nullptr, FT_LIST_NUMBER_ONLY);
    if (FT_Result == FT_OK) {
        return true;
    }
    return false;
}
