#ifndef DESKTOPSERVICES_H
#define DESKTOPSERVICES_H

#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QString)

namespace DesktopUtils {

bool openLocalFileOrDir(const QString &path);

}

#endif // DESKTOPSERVICES_H
