#include "ftdilibraries.h"

#include <atomic>
#include <mutex>
#include <system_error>
#include <vector>

#include "sharedlibrary.h"

#ifdef _WIN32
#include <windows.h> /*! GetModuleHandleExW, GetModuleFileNameW */
#endif

/*! File names of the libraries. They can be changed from CMake
 *  (FTDI_UTILS_FTD2XX_LIBNAME, FTDI_UTILS_MPSSE_LIBNAME, FTDI_UTILS_MPSSE_LIBNAME_DEBUG). */
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

namespace {

/*! Protects all the variables below, except the two atomic flags.
 *  Lock order: Ftd2xxWrapper's global mutex (if needed) BEFORE this one, never the opposite.
 *  Taking two mutexes always in the same order makes deadlocks impossible. */
std::mutex loaderMutex;

SharedLibrary ftd2xxLibrary;
SharedLibrary mpsseLibrary;

Ftd2xxFunctions ftd2xxFunctions;
MpsseFunctions mpsseFunctions;


/*
 * Atomic flags - read without taking loaderMutex by every FTDI call (thousands of requests
 * per second are performed)
 * Multiple threads may read while another might be loading the lib.
 * std::atomic guarantees that the read and the write are indivisible and that everything
 * written before "flag = true" (the function pointers) is visible to a thread that reads
 * "flag == true".
 */
std::atomic<bool> ftd2xxLoaded{false};
std::atomic<bool> mpsseLoaded{false};

std::filesystem::path libraryDirectory;
FtdiLoadError loadErrorType = FtdiLoadError::None;
std::string loadErrorMessage;

#ifdef _WIN32
/*! Any object of this module: its address tells GetModuleHandleExW which module contains ftdi_utils */
const int probe = 0;

/*! Folder of the module that contains ftdi_utils: the executable when it is linked statically in an
 *  application (EMCR), the DLL when it is linked in a DLL (e.g. e384commlib_labview.dll, loaded by
 *  LabVIEW.exe from another folder). Empty if it cannot be found. */
std::filesystem::path folderOfThisModule() {
    HMODULE module = nullptr;
    const DWORD flags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;
    if (!GetModuleHandleExW(flags, reinterpret_cast<LPCWSTR>(&probe), &module)) {
        return {};
    }

    /*! GetModuleFileNameW truncates silently: grow the buffer until the whole path fits */
    std::vector<wchar_t> buffer(MAX_PATH);
    while (true) {
        DWORD length = GetModuleFileNameW(module, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) {
            return {};
        }
        if (length < buffer.size()) {
            return std::filesystem::path(std::wstring(buffer.data(), length)).parent_path();
        }
        buffer.resize(buffer.size() * 2);
    }
}
#endif

/* Full path of a library.
 * - Directory set with setLibraryDirectory: only that folder.
 * - Otherwise, on Windows: the folder of the module containing ftdi_utils, if the file is there
 *   (needed when ftdi_utils is inside a DLL loaded by another program, e.g. the LabVIEW wrapper:
 *   the default search would look only in the folder of LabVIEW.exe).
 * - Otherwise just the file name: default search (folder of the executable, then System32). */
std::filesystem::path libraryPath(const char *fileName) {
    if (!libraryDirectory.empty()) {
        return libraryDirectory / fileName;
    }

#ifdef _WIN32
    std::filesystem::path moduleFolder = folderOfThisModule();
    if (!moduleFolder.empty()) {
        std::filesystem::path candidate = moduleFolder / fileName;
        std::error_code error;
        if (std::filesystem::exists(candidate, error)) {
            return candidate;
        }
    }
#endif

    return std::filesystem::path(fileName);
}

void setError(FtdiLoadError type, const std::string &message) {
    loadErrorType = type;
    loadErrorMessage = message;
}

void clearError() {
    setError(FtdiLoadError::None, "");
}

/*! Finds a function in the library and stores its address in "function".
 *  If the library does not export it, its name is added to "missingNames". */
template <typename Function>
void findFunction(const SharedLibrary &library, const char *name,
                  Function *&function, std::vector<std::string> &missingNames) {
    void *address = library.findSymbol(name);
    function = reinterpret_cast<Function *>(address);
    if (function == nullptr) {
        missingNames.push_back(name);
    }
}

std::string joinNames(const std::vector<std::string> &names) {
    std::string text;
    for (const std::string &name : names) {
        if (!text.empty()) {
            text += ", ";
        }
        text += name;
    }
    return text;
}

std::vector<std::string> findFtd2xxFunctions(Ftd2xxFunctions &functions) {
    std::vector<std::string> missingNames;
    findFunction(ftd2xxLibrary, "FT_ListDevices", functions.ListDevices, missingNames);
    findFunction(ftd2xxLibrary, "FT_CreateDeviceInfoList", functions.CreateDeviceInfoList, missingNames);
    findFunction(ftd2xxLibrary, "FT_GetDeviceInfoList", functions.GetDeviceInfoList, missingNames);
    findFunction(ftd2xxLibrary, "FT_Open", functions.Open, missingNames);
    findFunction(ftd2xxLibrary, "FT_OpenEx", functions.OpenEx, missingNames);
    findFunction(ftd2xxLibrary, "FT_Close", functions.Close, missingNames);
    findFunction(ftd2xxLibrary, "FT_SetBitMode", functions.SetBitMode, missingNames);
    findFunction(ftd2xxLibrary, "FT_SetLatencyTimer", functions.SetLatencyTimer, missingNames);
    findFunction(ftd2xxLibrary, "FT_SetUSBParameters", functions.SetUSBParameters, missingNames);
    findFunction(ftd2xxLibrary, "FT_SetFlowControl", functions.SetFlowControl, missingNames);
    findFunction(ftd2xxLibrary, "FT_GetQueueStatus", functions.GetQueueStatus, missingNames);
    findFunction(ftd2xxLibrary, "FT_Write", functions.Write, missingNames);
    findFunction(ftd2xxLibrary, "FT_Read", functions.Read, missingNames);
    findFunction(ftd2xxLibrary, "FT_Purge", functions.Purge, missingNames);
    findFunction(ftd2xxLibrary, "FT_ResetDevice", functions.ResetDevice, missingNames);
    findFunction(ftd2xxLibrary, "FT_ReadEE", functions.ReadEE, missingNames);
    return missingNames;
}

std::vector<std::string> findMpsseFunctions(MpsseFunctions &functions) {
    std::vector<std::string> missingNames;

    /*! Optional functions: older libMPSSE versions do not export them, a missing one is not an error */
    std::vector<std::string> ignoredNames;
    findFunction(mpsseLibrary, "Init_libMPSSE", functions.Init, ignoredNames);
    findFunction(mpsseLibrary, "Cleanup_libMPSSE", functions.Cleanup, ignoredNames);

    findFunction(mpsseLibrary, "SPI_GetNumChannels", functions.GetNumChannels, missingNames);
    findFunction(mpsseLibrary, "SPI_GetChannelInfo", functions.GetChannelInfo, missingNames);
    findFunction(mpsseLibrary, "SPI_OpenChannel", functions.OpenChannel, missingNames);
    findFunction(mpsseLibrary, "SPI_InitChannel", functions.InitChannel, missingNames);
    findFunction(mpsseLibrary, "SPI_CloseChannel", functions.CloseChannel, missingNames);
    findFunction(mpsseLibrary, "SPI_Read", functions.Read, missingNames);
    findFunction(mpsseLibrary, "SPI_Write", functions.Write, missingNames);
    findFunction(mpsseLibrary, "FT_WriteGPIO", functions.WriteGPIO, missingNames);
    findFunction(mpsseLibrary, "FT_ReadGPIO", functions.ReadGPIO, missingNames);
    return missingNames;
}

/* Loads FTD2XX. The caller must hold loaderMutex. */
bool loadFtd2xxLocked() {
    if (ftd2xxLoaded) {
        return true;
    }

    /* Load the file */
    if (!ftd2xxLibrary.open(libraryPath(FTDI_UTILS_FTD2XX_LIBNAME))) {
        setError(FtdiLoadError::LibraryNotFound, ftd2xxLibrary.errorMessage());
        return false;
    }

    /* Find all the functions */
    Ftd2xxFunctions functions;
    std::vector<std::string> missingNames = findFtd2xxFunctions(functions);
    if (!missingNames.empty()) {
        setError(FtdiLoadError::SymbolMissing,
                 std::string(FTDI_UTILS_FTD2XX_LIBNAME) + " does not export: " + joinNames(missingNames));
        ftd2xxLibrary.close();
        return false;
    }

    /* Mmake the functions available to the other threads.
     * Order matters: first the pointers, then the flag (see the comment on ftd2xxLoaded). */
    ftd2xxFunctions = functions;
    clearError();
    ftd2xxLoaded = true;
    return true;
}

/*! Loads libMPSSE. The caller must hold loaderMutex. */
bool loadMpsseLocked() {
    if (mpsseLoaded) {
        return true;
    }

    /* libMPSSE depends on FTD2XX: load FTD2XX first, so that libMPSSE uses the same copy */
    if (!loadFtd2xxLocked()) {
        return false;
    }

    /* load the file */
    if (!mpsseLibrary.open(libraryPath(FTDI_UTILS_MPSSE_LIBNAME))) {
        setError(FtdiLoadError::LibraryNotFound, mpsseLibrary.errorMessage());
        return false;
    }

    /* find all the functions */
    MpsseFunctions functions;
    std::vector<std::string> missingNames = findMpsseFunctions(functions);
    if (!missingNames.empty()) {
        setError(FtdiLoadError::SymbolMissing,
                 std::string(FTDI_UTILS_MPSSE_LIBNAME) + " does not export: " + joinNames(missingNames));
        mpsseLibrary.close();
        return false;
    }

    /* make the functions available (first the pointers, then the flag) */
    mpsseFunctions = functions;
    clearError();
    mpsseLoaded = true;
    return true;
}

} // namespace

bool FtdiLibraries::loadFtd2xx() {
    /* Usual case: already loaded, no mutex needed (see the comment on ftd2xxLoaded) */
    if (ftd2xxLoaded) {
        return true;
    }
    std::lock_guard<std::mutex> lock(loaderMutex);
    return loadFtd2xxLocked();
}

bool FtdiLibraries::loadMpsse() {
    if (mpsseLoaded) {
        return true;
    }
    std::lock_guard<std::mutex> lock(loaderMutex);
    return loadMpsseLocked();
}

void FtdiLibraries::unload() {
    std::lock_guard<std::mutex> lock(loaderMutex);

    /* First the flags, so that nobody starts using the functions any more */
    mpsseLoaded = false;
    ftd2xxLoaded = false;

    /* libMPSSE first, because it depends on FTD2XX */
    mpsseLibrary.close();
    ftd2xxLibrary.close();

    mpsseFunctions = MpsseFunctions();
    ftd2xxFunctions = Ftd2xxFunctions();
}

bool FtdiLibraries::isFtd2xxLoaded() {
    return ftd2xxLoaded;
}

bool FtdiLibraries::isMpsseLoaded() {
    return mpsseLoaded;
}

const Ftd2xxFunctions &FtdiLibraries::ftd2xx() {
    return ftd2xxFunctions;
}

const MpsseFunctions &FtdiLibraries::mpsse() {
    return mpsseFunctions;
}

void FtdiLibraries::setLibraryDirectory(const std::filesystem::path &directory) {
    std::lock_guard<std::mutex> lock(loaderMutex);

    if (directory.empty()) {
        libraryDirectory.clear();
        return;
    }

    /*! Windows needs an absolute path to also search the dependencies in the same folder.
     *  If the conversion fails, the path is used as it is. */
    std::error_code error;
    std::filesystem::path absoluteDirectory = std::filesystem::absolute(directory, error);
    if (error) {
        libraryDirectory = directory;
    } else {
        libraryDirectory = absoluteDirectory;
    }

    /* Use the native separators ('\\' on Windows) also in the error messages */
    libraryDirectory.make_preferred();
}

FtdiLoadError FtdiLibraries::lastErrorType() {
    std::lock_guard<std::mutex> lock(loaderMutex);
    return loadErrorType;
}

std::string FtdiLibraries::lastErrorMessage() {
    std::lock_guard<std::mutex> lock(loaderMutex);
    return loadErrorMessage;
}
