#ifndef QT_UTILITIES_CONVERSION_H
#define QT_UTILITIES_CONVERSION_H

#include "../global.h"

#include <QString>

#include <string>

namespace QtUtilities {

inline QByteArray toNativeFileName(const QString &fileName)
{
#if !defined(PLATFORM_MINGW) || !defined(CPP_UTILITIES_USE_NATIVE_FILE_BUFFER)
    return fileName.toLocal8Bit();
#else
    return fileName.toUtf8();
#endif
}

inline QString fromNativeFileName(const char *nativeFileName, int size = -1)
{
#if !defined(PLATFORM_MINGW) || !defined(CPP_UTILITIES_USE_NATIVE_FILE_BUFFER)
    return QString::fromLocal8Bit(nativeFileName, size);
#else
    return QString::fromUtf8(nativeFileName, size);
#endif
}

inline QString fromNativeFileName(const std::string &nativeFileName)
{
#if !defined(PLATFORM_MINGW) || !defined(CPP_UTILITIES_USE_NATIVE_FILE_BUFFER)
    return QString::fromLocal8Bit(nativeFileName.data(), static_cast<int>(nativeFileName.size()));
#else
    return QString::fromUtf8(nativeFileName.data(), static_cast<int>(nativeFileName.size()));
#endif
}

} // namespace QtUtilities

#endif // QT_UTILITIES_CONVERSION_H
