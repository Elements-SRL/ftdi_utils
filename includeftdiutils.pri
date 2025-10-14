INCLUDEPATH += $$(FTDI_UTILS_PATH)include
DEPENDPATH += $$(FTDI_UTILS_PATH)include

msvc:win32:CONFIG(release, debug|release): LIBS += -L$$(FTDI_UTILS_PATH)lib/release/ -lftdi_utils
else:msvc:win32:CONFIG(debug, debug|release): LIBS += -L$$(FTDI_UTILS_PATH)lib/debug/ -lftdi_utilsd

include($$(FTD2XX_PATH)includeftd2xx.pri)
include($$(LIBMPSSE_PATH)includelibmpsse.pri)
