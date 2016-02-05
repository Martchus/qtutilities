#ifndef DIALOGS_DIALOGUTILS_H
#define DIALOGS_DIALOGUTILS_H

#include <c++utilities/application/global.h>

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

QString LIB_EXPORT generateWindowTitle(DocumentStatus documentStatus, const QString &documentPath);

#ifndef GUI_NONE
# ifdef Q_OS_WIN32
QColor LIB_EXPORT windowFrameColor();
QColor LIB_EXPORT instructionTextColor();
# endif
const QString LIB_EXPORT &dialogStyle();
void LIB_EXPORT makeHeading(QWidget *widget);
void LIB_EXPORT updateStyle(QWidget *widget);
#endif

} // namespace Dialogs

#endif // DIALOGS_DIALOGUTILS_H
