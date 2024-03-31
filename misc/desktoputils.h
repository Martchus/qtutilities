#ifndef DESKTOP_UTILS_DESKTOPSERVICES_H
#define DESKTOP_UTILS_DESKTOPSERVICES_H

#include "../global.h"

#include <QMetaObject>
#include <QPalette>

#include <functional>
#include <optional>

QT_FORWARD_DECLARE_CLASS(QObject)
QT_FORWARD_DECLARE_CLASS(QString)

namespace QtUtilities {

QT_UTILITIES_EXPORT bool openLocalFileOrDir(const QString &path);
QT_UTILITIES_EXPORT bool isPaletteDark(const QPalette &palette = QPalette());
QT_UTILITIES_EXPORT std::optional<bool> isDarkModeEnabled();
QT_UTILITIES_EXPORT QMetaObject::Connection onDarkModeChanged(
    std::function<void(bool)> &&darkModeChangedCallback, QObject *context = nullptr, bool invokeImmediately = true);

} // namespace QtUtilities

#endif // DESKTOP_UTILS_DESKTOPSERVICES_H
