#ifndef DESKTOP_UTILS_DESKTOPSERVICES_H
#define DESKTOP_UTILS_DESKTOPSERVICES_H

#include "../global.h"

#include <QPalette>

QT_FORWARD_DECLARE_CLASS(QString)

namespace QtUtilities {

QT_UTILITIES_EXPORT bool openLocalFileOrDir(const QString &path);
QT_UTILITIES_EXPORT bool isPaletteDark(const QPalette &palette = QPalette());

} // namespace QtUtilities

#endif // DESKTOP_UTILS_DESKTOPSERVICES_H
