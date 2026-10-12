#include "sharedlibrary.h"

#include <system_error>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

/*! Path as UTF-8 text, for the error messages.
 *  path::string() is not used because on Windows it converts to the local code page and throws
 *  an exception if the path contains characters that the code page cannot represent. */
static std::string pathToText(const std::filesystem::path &path) {
    const std::u8string utf8 = path.u8string();
    return std::string(utf8.begin(), utf8.end());
}

#ifdef _WIN32

bool SharedLibrary::open(const std::filesystem::path &file) {
    close();
    lastError.clear();

    /*! Where Windows looks for the library (and for the libraries it depends on):
     *  LOAD_LIBRARY_SEARCH_DEFAULT_DIRS = folder of the executable + System32.
     *  The current directory and the PATH are deliberately excluded. */
    DWORD searchFlags = LOAD_LIBRARY_SEARCH_DEFAULT_DIRS;

    if (file.has_parent_path()) {
        /*! A full path was given: also look for the dependencies in the folder of the library.
         *  LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR requires an absolute path. */
        searchFlags |= LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR;
    }

    /*! std::filesystem::path stores the path as UTF-16 on Windows: c_str() is the
     *  wchar_t string expected by LoadLibraryExW, no manual conversion is needed */
    HMODULE module = LoadLibraryExW(file.c_str(), nullptr, searchFlags);

    if (module == nullptr) {
        /*! GetLastError() must be read immediately, before any other system call.
         *  std::system_category().message() translates the Windows error code into text
         *  (it uses FormatMessage internally) */
        const DWORD errorCode = GetLastError();
        lastError = "cannot load " + pathToText(file)
                + ": error " + std::to_string(errorCode)
                + " (" + std::system_category().message(static_cast<int>(errorCode)) + ")";
        return false;
    }

    handle = module;
    return true;
}

void SharedLibrary::close() {
    if (handle != nullptr) {
        FreeLibrary(static_cast<HMODULE>(handle));
        handle = nullptr;
    }
}

void *SharedLibrary::findSymbol(const char *name) const {
    if (handle == nullptr) {
        return nullptr;
    }
    FARPROC address = GetProcAddress(static_cast<HMODULE>(handle), name);
    return reinterpret_cast<void *>(address);
}

#else // Linux

bool SharedLibrary::open(const std::filesystem::path &file) {
    close();
    lastError.clear();

    /*! RTLD_NOW: resolve all the symbols immediately, so that a broken library fails here
     *            and not later, in the middle of a call.
     *  RTLD_GLOBAL: libmpsse uses the FT_* functions of the libftd2xx already loaded. */
    handle = dlopen(file.c_str(), RTLD_NOW | RTLD_GLOBAL);

    if (handle == nullptr) {
        const char *description = dlerror();
        lastError = "cannot load " + pathToText(file) + ": "
                + (description != nullptr ? description : "unknown error");
        return false;
    }
    return true;
}

void SharedLibrary::close() {
    if (handle != nullptr) {
        dlclose(handle);
        handle = nullptr;
    }
}

void *SharedLibrary::findSymbol(const char *name) const {
    if (handle == nullptr) {
        return nullptr;
    }
    return dlsym(handle, name);
}

#endif

bool SharedLibrary::isOpen() const {
    return handle != nullptr;
}

const std::string &SharedLibrary::errorMessage() const {
    return lastError;
}
