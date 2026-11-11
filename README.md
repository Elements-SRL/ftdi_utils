# FTDI Utilities

This project provides a static C++ utility library used by the Elements communication libraries (e384commLib) to talk to
devices with an FTDI USB interface. It provides:

- `Ftd2xxWrapper`: thread-safe access to the FTDI libraries **FTD2XX** (D2XX driver API) and
  **libMPSSE** (SPI over FTDI), **loaded at runtime**;
- `FtdiEeprom*`, `FtdiCalibrationEeprom`: access to the EEPROMs of the Elements devices.

FTDI DLL libraries are **lazy loaded** only when required by some plugged-in device. 

## REQUIREMENTS
Compatibility has been verified for the following systems:
- Windows 10+
- Linux Arm

Note: Elements only provides free support for Windows 11+.

- C++20
- CMake 3.27 or newer (also for the projects that use ftdi_utils, see [Using ftdi_utils](#using-ftdi_utils-in-a-cmake-project))

## HOW THE FTDI LIBRARIES ARE LOADED

ftdi_utils does **not** link FTD2XX and libMPSSE. They are loaded at runtime
(`LoadLibraryExW` on Windows, `dlopen` on Linux) only when they are needed. Consequences:

- an application using ftdi_utils starts and works (with its other devices) even when the
  FTDI DLLs are missing;
- the FTDI DLLs are loaded only on PCs where an FTDI device is actually used.

## Loading schema
| Library | Loaded by |
|---|---|
| FTD2XX | the first `FTW_*` call that needs it (listing, opening a device), or `Ftd2xxWrapper::loadFtd2xx()` |
| libMPSSE | the first `SPIW_*` call or `FTW_WriteGPIO` / `FTW_ReadGPIO`, or `Ftd2xxWrapper::loadMpsse()`. FTD2XX is loaded first, because libMPSSE depends on it |

`Ftd2xxWrapper::getDeviceCount()`, used by the device detection, first checks whether a USB
device with the FTDI vendor id (0x0403) is plugged in, **without** using FTD2XX
(Windows: Configuration Manager, Linux: `/sys/bus/usb/devices`). If there is none, nothing is
loaded.

## DLL FILES

| | Windows Release | Windows Debug | Linux |
|---|---|---|---|
| FTD2XX | `FTD2XX.dll` | `FTD2XX.dll` | `libftd2xx.so` |
| libMPSSE | `MPSSE.dll` | `MPSSEd.dll` | `libmpsse.so` |

The names can be changed when configuring ftdi_utils with the CMake cache variables
`FTDI_UTILS_FTD2XX_LIBNAME`, `FTDI_UTILS_MPSSE_LIBNAME`, `FTDI_UTILS_MPSSE_LIBNAME_DEBUG`.

Note: the FTDI package ships the 64-bit library as `FTD2XX64.dll`: distribute it as `FTD2XX.dll`.

### Where (Windows)

By default the libraries are searched **only** in:
1. the folder of the module that contains ftdi_utils, if the DLL is there. It is the folder of the
   executable when ftdi_utils is linked in an application (EMCR), and the folder of the DLL when it
   is linked in a DLL loaded by another program (e.g. `e384commlib_labview.dll` loaded by LabVIEW);
2. the folder of the executable;
3. the system folder (`System32`), where the FTDI driver installs `FTD2XX.dll`.

The current directory and the `PATH` are **never** searched: a DLL with the same name placed
somewhere else cannot be loaded by mistake, and development builds behave like installed ones.
During development the DLLs must therefore be copied next to the executable
(see [the post-build example](#copying-the-ftdi-dlls-next-to-the-executable)).

`Ftd2xxWrapper::setLibraryDirectory(folder)` restricts the search to a single folder (used by
the tests and by the e384commLib simulation of missing libraries).

## Unloading

`Ftd2xxWrapper::unloadLibraries()` unloads both libraries, but refuses (returns `false`) while
any FTDI handle is open: a thread could otherwise call a function that no longer exists.
The libraries are never unloaded automatically, not even at program exit (Windows releases
them when the process ends).

## ERROR HANDLING

When a library cannot be loaded:

- the `FTW_*` / `SPIW_*` functions return `FT_OTHER_ERROR` (`SPIW_OpenChannelBySerial` returns
  `nullptr`, `getDeviceCount` returns `false`);
- `Ftd2xxWrapper::getLastLoadErrorType()` tells why:
  - `FtdiLoadError::LibraryNotFound`: the file could not be loaded (missing, wrong
    architecture, missing dependency);
  - `FtdiLoadError::SymbolMissing`: the file does not export all the needed functions
    (wrong or too old version);
- `Ftd2xxWrapper::getLastLoadError()` returns a description for logs and error dialogs, e.g.
  `cannot load FTD2XX.dll: error 126 (The specified module could not be found.)`.

A failed load is retried at the next call, so copying the DLLs while the application is
running is enough to recover.

```cpp
DWORD numDevs = 0;
if (!Ftd2xxWrapper::getDeviceCount(numDevs)) {
    if (!Ftd2xxWrapper::isFtd2xxLoaded()) {
        showError("FTDI driver library not available", Ftd2xxWrapper::getLastLoadError());
    }
}
```

## THREAD SAFETY

All the `Ftd2xxWrapper` functions can be called from any thread.

- Functions without a handle (list, open, close, SPI channel open/close) are serialized by a
  global mutex: FTD2XX is not safe when the device list is read and changed concurrently.
- Functions with a handle (read, write, purge, ...) are serialized **per handle**: different
  devices can be used in parallel, the same device by one thread at a time.
- The "library loaded" flags are `std::atomic<bool>`: every call checks them without taking a
  mutex (the read thread calls `FTW_Read` thousands of times per second), while the atomic
  guarantees that a thread that sees "loaded" also sees the function pointers written before.
- Mutexes are always taken in the same order (global, then loader) so that deadlocks cannot happen.

## PROJECT FILES

| File | Content |
|---|---|
| `src/ftd2xxwrapper.h/.cpp` | Public API (`Ftd2xxWrapper`, `FtdiLoadError`) |
| `src/internal/ftdilibraries.*` | Runtime loading of FTD2XX and libMPSSE, function pointer tables |
| `src/internal/sharedlibrary.*` | Small wrapper around `LoadLibraryExW` / `dlopen` |
| `src/internal/handleregistry.*` | Per-handle mutexes, open handles, `HandleLock` |
| `src/internal/usbdevicepresence.*` | USB vendor id check without FTD2XX |
| `src/ftdieeprom*`, `src/ftdicalibrationeeprom*` | EEPROMs of the Elements devices |

The files in `src/internal` are not installed: they are not part of the public API.

## BUILDING

### Toolchain (Windows)
Elements builds the ftdi_utils statically using Qt Creator with the Qt libraries 6.7.3 and MSVC 2022 [Qt Community](https://www.qt.io/download-dev)

Note: the Qt Libraries are not used. Qt Creator is used for legacy reasons.

### Environment variables

| Variable | Used for |
|---|---|
| `LIBMPSSE_PATH` | Folder of the MPSSE CMake package (`MPSSEConfig.cmake`): only the FTDI headers are used |
| `FTDI_UTILS_PATH` | Install folder of ftdi_utils (default `CMAKE_INSTALL_PREFIX`) |

### CMake options

| Option | Default | |
|---|---|---|
| `FTDI_UTILS_FTD2XX_LIBNAME` | `FTD2XX.dll` / `libftd2xx.so` | File name of FTD2XX |
| `FTDI_UTILS_MPSSE_LIBNAME` | `MPSSE.dll` / `libmpsse.so` | File name of libMPSSE (Release) |
| `FTDI_UTILS_MPSSE_LIBNAME_DEBUG` | `MPSSEd.dll` / `libmpsse.so` | File name of libMPSSE (Debug) |

## Install

Build and install **both Debug and Release**: they are installed side by side
(`lib/ftdi_utils.lib`, `lib/ftdi_utilsd.lib`, `cmake/ftdi_utilsTargets-debug.cmake`,
`cmake/ftdi_utilsTargets-release.cmake`). If only Release is installed, a Debug project that
uses ftdi_utils links the Release library and fails with `LNK2038 ... _ITERATOR_DEBUG_LEVEL`.

### USING FTDI_UTILS IN A CMAKE PROJECT

```cmake
cmake_minimum_required(VERSION 3.27)   # the exported target uses $<COMPILE_ONLY:...>

list(APPEND CMAKE_PREFIX_PATH "$ENV{FTDI_UTILS_PATH}")
find_package(ftdi_utils 1.6 REQUIRED CONFIG)   # also finds MPSSE through LIBMPSSE_PATH

target_link_libraries(my_app PRIVATE ftdi_utils::ftdi_utils)
```

Neither `ftd2xx.lib` nor `MPSSE.lib` is linked: the executable does not import the FTDI DLLs
(you can check with `dumpbin /dependents my_app.exe`).

### Copying the FTDI DLLs next to the executable

Since the DLLs are not linked, neither the linker nor Qt Creator knows about them. For
development builds copy them next to the executable after each build:

```cmake
if(WIN32)
    add_custom_command(TARGET my_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:MPSSE::MPSSE> $<TARGET_FILE_DIR:my_app>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:ftd2xx::ftd2xx> $<TARGET_FILE_DIR:my_app>/FTD2XX.dll
    )
endif()
```

The installer distributes the DLLs separately (see the licensing note below).

## DRIVERS

### Windows
Install EDR4 to make sure all the needed drivers are correctly installed [elements-ic.com](https://elements-ic.com/downloads/)
Disclaimer: EDR4 is officially supported only on windows, hence if you want to build your library for other operating systems/architectures you'll have to manually search and install the needed drivers.

### Linux
The D2XX drivers used by the commlib are available at the following link: [ftdichip.com](https://ftdichip.com/drivers/d2xx-drivers/)

Choose the compatible library for your processor and follow the instructions on the website.

For the FTDI driver to work correctly you'll also need to permanently override the kernel automatic drivers and use the FTDI's to communicate with usb ports.
To do so you can use the following lines of code.
```
sudo echo "blacklist ftdi_sio" >> /etc/modprobe.d/blacklist-ftdi.conf
sudo echo "blacklist usbserial" >> /etc/modprobe.d/blacklist-usbserial.conf
```

## DEPENDENCIES AND LICENSING
The ftdi_utils requires the ftd2xx library and the libmpsse, both distributed by FTDI [ftdichip.com](https://www.ftdichip.com)

Disclaimer: since FTDI does not explicitly grant the right to redistribute the source code or modified versions, the programmer has to get the code/binaries independently.

## PREBUILT BINARIES
Prebuilt versions of the library are available here: [link](https://drive.proton.me/urls/N3SMDKDD2G#VplThxx6N5XC)
