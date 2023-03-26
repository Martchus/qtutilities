#ifndef DIALOGS_DIALOGUTILS_H
#define DIALOGS_DIALOGUTILS_H

#include "../global.h"

#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QWidget)
QT_FORWARD_DECLARE_CLASS(QColor)
QT_FORWARD_DECLARE_CLASS(QPalette)
QT_FORWARD_DECLARE_CLASS(QPoint)
QT_FORWARD_DECLARE_CLASS(QRect)

namespace QtUtilities {

/*!
 * \brief The DocumentStatus enum specifies the status of the document in a
 * window.
 */
enum class DocumentStatus {
    NoDocument, /**< There is no document opened. The document path is ignored in
                 this case. */
    Saved, /**< There is a document opened. All modifications have been saved yet.
            */
    Unsaved /**< There is a document opened and there are unsaved modifications.
             */
};

QT_UTILITIES_EXPORT QString generateWindowTitle(DocumentStatus documentStatus, const QString &documentPath);

#if defined(QT_UTILITIES_GUI_QTWIDGETS) || defined(QT_UTILITIES_GUI_QTQUICK)
#ifdef Q_OS_WINDOWS
[[deprecated]] QT_UTILITIES_EXPORT QColor windowFrameColor();
QT_UTILITIES_EXPORT QColor windowFrameColorForPalette(const QPalette &palette);
[[deprecated]] QT_UTILITIES_EXPORT QColor instructionTextColor();
QT_UTILITIES_EXPORT QColor instructionTextColorForPalette(const QPalette &palette);
#endif
[[deprecated]] QT_UTILITIES_EXPORT const QString &dialogStyle();
QT_UTILITIES_EXPORT QString dialogStyleForPalette(const QPalette &palette);
#ifdef QT_UTILITIES_GUI_QTWIDGETS
QT_UTILITIES_EXPORT QRect availableScreenGeometryAtPoint(const QPoint &point);
QT_UTILITIES_EXPORT void centerWidget(QWidget *widget, const QWidget *parent = nullptr, const QPoint *position = nullptr);
QT_UTILITIES_EXPORT bool centerWidgetAvoidingOverflow(QWidget *widget, const QWidget *parent = nullptr, const QPoint *position = nullptr);
QT_UTILITIES_EXPORT void cornerWidget(QWidget *widget, const QPoint *position = nullptr);
QT_UTILITIES_EXPORT void makeHeading(QWidget *widget);
QT_UTILITIES_EXPORT void updateStyle(QWidget *widget);
#endif
#endif

} // namespace QtUtilities

#endif // DIALOGS_DIALOGUTILS_H
