cmake_minimum_required(VERSION 3.21)

project(ftdi_utils
    VERSION 1.4.0
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

target_link_libraries(ftdi_utils
    PUBLIC
    MPSSE::MPSSE
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
    DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake/\${CMAKE_INSTALL_CONFIG_NAME}"
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
