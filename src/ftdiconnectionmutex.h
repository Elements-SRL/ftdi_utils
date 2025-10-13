#ifndef FTDICONNECTIONMUTEX_H
#define FTDICONNECTIONMUTEX_H

#include "ftdi_utils_global.h"

#include <mutex>
extern FTDIUTILSSHARED_EXPORT std::mutex ftdiConnectionMutex;

#endif // FTDICONNECTIONMUTEX_H
