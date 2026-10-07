cmake_minimum_required(VERSION 3.27) # $<COMPILE_ONLY:...>

project(ftdi_utils
    VERSION 1.6.0
    LANGUAGES CXX
)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# Library

add_library(ftdi_utils STATIC
    src/ftd2xxwrapper.cpp
    src/ftdicalibrationeeprom.cpp
    src/ftdieeprom.cpp
    src/ftdieeprom56.cpp
    src/ftdieepromdemo.cpp
)

add_library(ftdi_utils::ftdi_utils ALIAS ftdi_utils)

# C standard

set_target_properties(ftdi_utils PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Headers

target_include_directories(ftdi_utils
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Defines

target_compile_definitions(ftdi_utils
    PUBLIC
    FTDI_UTILS_STATIC
    FTDIUTILS_VER_MAJOR=${PROJECT_VERSION_MAJOR}
    FTDIUTILS_VER_MINOR=${PROJECT_VERSION_MINOR}
    FTDIUTILS_VER_PATCH=${PROJECT_VERSION_PATCH}
)

# Dependencies

cmake_path(SET LIBMPSSE_PATH_CMAKE "$ENV{LIBMPSSE_PATH}")
list(APPEND CMAKE_PREFIX_PATH "${LIBMPSSE_PATH_CMAKE}")

find_package(MPSSE REQUIRED CONFIG)

# ftd2xx and libMPSSE are loaded at runtime (LoadLibrary/dlopen, see ftd2xxwrapper.cpp):
# only their headers are needed, nothing is linked. Consumers no longer import
# FTD2XX.dll / MPSSE.dll, so the application starts even when the DLLs are missing.
target_link_libraries(ftdi_utils
    PUBLIC
    $<COMPILE_ONLY:MPSSE::MPSSE>
    PRIVATE
    $<$<PLATFORM_ID:Windows>:cfgmgr32>   # device presence check (CM_Get_Device_ID_List)
    ${CMAKE_DL_LIBS}                     # dlopen on Linux
)

# Runtime library names (override with -D... if the DLLs have different names)
if(WIN32)
    set(FTDI_UTILS_FTD2XX_LIBNAME "FTD2XX.dll" CACHE STRING "ftd2xx library loaded at runtime")
    set(FTDI_UTILS_MPSSE_LIBNAME "MPSSE.dll" CACHE STRING "libMPSSE library loaded at runtime (Release)")
    set(FTDI_UTILS_MPSSE_LIBNAME_DEBUG "MPSSEd.dll" CACHE STRING "libMPSSE library loaded at runtime (Debug)")
else()
    set(FTDI_UTILS_FTD2XX_LIBNAME "libftd2xx.so" CACHE STRING "ftd2xx library loaded at runtime")
    set(FTDI_UTILS_MPSSE_LIBNAME "libmpsse.so" CACHE STRING "libMPSSE library loaded at runtime (Release)")
    set(FTDI_UTILS_MPSSE_LIBNAME_DEBUG "libmpsse.so" CACHE STRING "libMPSSE library loaded at runtime (Debug)")
endif()

target_compile_definitions(ftdi_utils
    PRIVATE
    "FTDI_UTILS_FTD2XX_LIBNAME=\"${FTDI_UTILS_FTD2XX_LIBNAME}\""
    "FTDI_UTILS_MPSSE_LIBNAME=\"$<IF:$<CONFIG:Debug>,${FTDI_UTILS_MPSSE_LIBNAME_DEBUG},${FTDI_UTILS_MPSSE_LIBNAME}>\""
)

# output debug/release

set_target_properties(ftdi_utils PROPERTIES
    DEBUG_POSTFIX d
)

# Install

cmake_path(SET FTDI_UTILS_PATH_CMAKE "$ENV{FTDI_UTILS_PATH}")
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX ${FTDI_UTILS_PATH_CMAKE} CACHE PATH "Install path" FORCE)
endif()

install(TARGETS ftdi_utils
    EXPORT ftdi_utilsTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install header

install(DIRECTORY src/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
)

# Export target

install(EXPORT ftdi_utilsTargets
    FILE ftdi_utilsTargets.cmake
    NAMESPACE ftdi_utils::
    DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake"
)

# Config package

configure_package_config_file(
    cmake/ftdi_utilsConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/ftdi_utilsConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_PREFIX}/cmake
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/ftdi_utilsConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/ftdi_utilsConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/ftdi_utilsConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_PREFIX}/cmake
)
