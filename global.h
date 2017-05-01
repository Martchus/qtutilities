// Created via CMake from template global.h.in
// WARNING! Any changes to this file will be overwritten by the next CMake run!

#ifndef QT_UTILITIES_GLOBAL
#define QT_UTILITIES_GLOBAL

#include <c++utilities/application/global.h>

#ifdef QT_UTILITIES_STATIC
#define QT_UTILITIES_EXPORT
#define QT_UTILITIES_IMPORT
#else
#define QT_UTILITIES_EXPORT LIB_EXPORT
#define QT_UTILITIES_IMPORT LIB_IMPORT
#endif

/*!
 * \def QT_UTILITIES_EXPORT
 * \brief Marks the symbol to be exported by the qtutilities library.
 */

/*!
 * \def QT_UTILITIES_IMPORT
 * \brief Marks the symbol to be imported from the qtutilities library.
 */

#endif // QT_UTILITIES_GLOBAL
