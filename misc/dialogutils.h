#ifndef DIALOGS_DIALOGUTILS_H
#define DIALOGS_DIALOGUTILS_H

#include <c++utilities/application/global.h>

#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QWidget)
QT_FORWARD_DECLARE_CLASS(QColor)

namespace Dialogs {

enum class DocumentStatus {
    NoDocument,
    Saved,
    Unsaved
};

QString LIB_EXPORT generateWindowTitle(DocumentStatus documentStatus, const QString &documentPath);

#ifndef GUI_NONE
#ifdef Q_OS_WIN32
QColor LIB_EXPORT windowFrameColor();
QColor LIB_EXPORT instructionTextColor();
#endif
const QString LIB_EXPORT &dialogStyle();
void LIB_EXPORT makeHeading(QWidget *widget);
#endif

} // namespace Dialogs

#endif // DIALOGS_DIALOGUTILS_H
