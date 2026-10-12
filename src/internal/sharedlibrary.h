#ifndef SHAREDLIBRARY_H
#define SHAREDLIBRARY_H

#include <filesystem>
#include <string>

/*! \brief Minimal wrapper around a shared library loaded at runtime (DLL on Windows, .so on Linux).
 *
 *  It hides the operating system calls (LoadLibraryExW / dlopen, GetProcAddress / dlsym,
 *  FreeLibrary / dlclose) so that the rest of the code does not need any #ifdef.
 *  The library is unloaded only by an explicit close(): it is NOT unloaded when the object is
 *  destroyed. The objects of ftdi_utils are static, so they are destroyed at program exit,
 *  when other threads or static objects could still be using the library. The operating system
 *  releases the library anyway when the process ends.
 *
 *  \note Internal to ftdi_utils: not installed with the public headers.
 */
class SharedLibrary {
public:
    SharedLibrary() = default;

    /*! A loaded library has a single owner: copying is not allowed */
    SharedLibrary(const SharedLibrary &) = delete;
    SharedLibrary &operator=(const SharedLibrary &) = delete;

    /*! \brief Loads the library.
     *
     *  \param file Library file name (e.g. "FTD2XX.dll") or full path.
     *  - Only a file name: on Windows the library is searched ONLY in the folder of the
     *    executable and in the system folder (System32). The current directory and the PATH
     *    are not searched, so a DLL with the same name placed somewhere else cannot be loaded
     *    by mistake. On Linux the standard dlopen search is used.
     *  - Full path: only that file is loaded. On Windows the dependencies of the library are
     *    searched in its folder as well.
     *  \return true on success, false otherwise (see errorMessage()).
     */
    bool open(const std::filesystem::path &file);

    /*! \brief Unloads the library. Does nothing if it is not loaded. */
    void close();

    bool isOpen() const;

    /*! \brief Address of an exported function, nullptr if the library does not export it. */
    void *findSymbol(const char *name) const;

    /*! \brief Description of the last open() failure, empty if the last open() succeeded. */
    const std::string &errorMessage() const;

private:
    void *handle = nullptr; /*!< HMODULE on Windows, dlopen handle on Linux */
    std::string lastError;
};

#endif // SHAREDLIBRARY_H
