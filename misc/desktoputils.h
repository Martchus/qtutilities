#ifndef DESKTOP_UTILS_DESKTOPSERVICES_H
#define DESKTOP_UTILS_DESKTOPSERVICES_H

#include "../global.h"

#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QString)

namespace DesktopUtils {

bool QT_UTILITIES_EXPORT openLocalFileOrDir(const QString &path);

}

#endif // DESKTOP_UTILS_DESKTOPSERVICES_H
