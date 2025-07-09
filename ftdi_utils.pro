QT     -= core gui
CONFIG -= qt

CONFIG(debug, debug|release) {
    QMAKE_CXXFLAGS += /Od
    TARGET = ftdi_utilsd
    DEFINES += DEBUG
}

CONFIG(release, debug|release) {
    QMAKE_CXXFLAGS += /O2
    TARGET = ftdi_utils
}

TEMPLATE = lib
#DEFINES += FTDI_UTILS_EXPORTS
DEFINES += FTDI_UTILS_STATIC
CONFIG += staticlib

CONFIG += c++17

include(version.pri)

DEFINES += "VERSION_MAJOR=$$VERSION_MAJOR"\
    "VERSION_MINOR=$$VERSION_MINOR"\
    "VERSION_PATCH=$$VERSION_PATCH"

VERSION_FULL = $${VERSION_MAJOR}.$${VERSION_MINOR}.$${VERSION_PATCH}

SOURCES += \
    src/ftdieeprom.cpp \
    src/ftdieeprom56.cpp \
    src/ftdieepromdemo.cpp

HEADERS += \
    src/ftdi_utils_global.h \
    src/ftdieeprom.h \
    src/ftdieeprom56.h \
    src/ftdieepromdemo.h

INCLUDEPATH += \
    ./src
DEPENDPATH += \
    ./src

include($$(FTD2XX_PATH)includeftd2xx.pri)
