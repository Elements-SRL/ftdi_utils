/*! \file ftdi_utils_global.h
 * \brief Defines global macros and typedefs.
 */
#ifndef FTDI_UTILS_GLOBAL_H
#define FTDI_UTILS_GLOBAL_H

/****************************\
 *  Shared library defines  *
\****************************/

#if defined(FTDI_UTILS_STATIC)
#  define FTDIUTILSSHARED_EXPORT
#else
#if defined(FTDI_UTILS_EXPORTS)
#  define FTDIUTILSSHARED_EXPORT __declspec(dllexport)
#else
#  define FTDIUTILSSHARED_EXPORT __declspec(dllimport)
#endif

#endif // FTDI_UTILS_GLOBAL_H
