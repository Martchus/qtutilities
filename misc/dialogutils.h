#ifndef DIALOGS_DIALOGUTILS_H
#define DIALOGS_DIALOGUTILS_H

#include "../global.h"

#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QWidget)
QT_FORWARD_DECLARE_CLASS(QColor)

namespace Dialogs {

/*!
 * \brief The DocumentStatus enum specifies the status of the document in a window.
 */
enum class DocumentStatus {
    NoDocument, /**< There is no document opened. The document path is ignored in this case. */
    Saved, /**< There is a document opened. All modifications have been saved yet. */
    Unsaved /**< There is a document opened and there are unsaved modifications. */
};

QString QT_UTILITIES_EXPORT generateWindowTitle(DocumentStatus documentStatus, const QString &documentPath);

#if defined(QT_UTILITIES_GUI_QTWIDGETS) || defined(QT_UTILITIES_GUI_QTQUICK)
#ifdef Q_OS_WIN32
QColor QT_UTILITIES_EXPORT windowFrameColor();
QColor QT_UTILITIES_EXPORT instructionTextColor();
#endif
const QString QT_UTILITIES_EXPORT &dialogStyle();
#ifdef QT_UTILITIES_GUI_QTWIDGETS
void QT_UTILITIES_EXPORT centerWidget(QWidget *widget);
void QT_UTILITIES_EXPORT cornerWidget(QWidget *widget);
void QT_UTILITIES_EXPORT makeHeading(QWidget *widget);
void QT_UTILITIES_EXPORT updateStyle(QWidget *widget);
#endif
#endif

} // namespace Dialogs

#endif // DIALOGS_DIALOGUTILS_H
