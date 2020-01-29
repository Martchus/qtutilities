#ifndef DESKTOP_UTILS_DESKTOPSERVICES_H
#define DESKTOP_UTILS_DESKTOPSERVICES_H

#include "../global.h"

#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QString)

namespace QtUtilities {

QT_UTILITIES_EXPORT bool openLocalFileOrDir(const QString &path);
}

#endif // DESKTOP_UTILS_DESKTOPSERVICES_H
