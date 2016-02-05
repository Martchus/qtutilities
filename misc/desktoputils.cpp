#include "desktoputils.h"

#include <c++utilities/application/global.h>

#include <QDesktopServices>
#include <QUrl>

namespace DesktopUtils {

/*!
 * \brief Shows the specified file or directory using the default file browser.
 * \remarks
 */
bool LIB_EXPORT openLocalFileOrDir(const QString &path)
{
#ifdef Q_OS_WIN32
    // backslashes are commonly used under Windows
    // -> replace backslashes with slashes to support Windows paths
    QString tmp(path);
    tmp.replace(QChar('\\'), QChar('/'));
    return QDesktopServices::openUrl(QUrl(QStringLiteral("file:///") + path, QUrl::TolerantMode));
#else
    return QDesktopServices::openUrl(QUrl(QStringLiteral("file://") + path, QUrl::TolerantMode));
#endif
}

}
