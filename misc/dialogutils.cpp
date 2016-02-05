#include "./dialogutils.h"

#ifdef GUI_NONE
# include <QCoreApplication>
#else
# include <QGuiApplication>
# include <QPalette>
# include <QWidget>
# include <QStyle>
#endif
#include <QFileInfo>
#include <QDir>

namespace Dialogs {

/*!
 * \brief Generates the window title string for the specified \a documentStatus
 *        and \a documentPath.
 */
QString generateWindowTitle(DocumentStatus documentStatus, const QString &documentPath)
{
    switch(documentStatus) {
    case DocumentStatus::Saved:
        if(documentPath.isEmpty()) {
            return QCoreApplication::translate("Utilities::windowTitle", "Unsaved - %1").arg(QCoreApplication::applicationName());
        } else {
            QFileInfo file(documentPath);
            return QCoreApplication::translate("Utilities::windowTitle", "%1 - %2 - %3").arg(file.fileName(), file.dir().path(), QCoreApplication::applicationName());
        }
    case DocumentStatus::Unsaved:
        if(documentPath.isEmpty()) {
            return QCoreApplication::translate("Utilities::windowTitle", "*Unsaved - %1").arg(QCoreApplication::applicationName());
        } else {
            QFileInfo file(documentPath);
            return QCoreApplication::translate("Utilities::windowTitle", "*%1 - %2 - %3").arg(file.fileName(), file.dir().path(), QCoreApplication::applicationName());
        }
    case DocumentStatus::NoDocument:
        return QCoreApplication::applicationName();
    default:
        return QString(); // to suppress warning: "control reaches end of non-void function"
    }
}

#ifndef GUI_NONE

# ifdef Q_OS_WIN32

/*!
 * \brief Returns the color used to draw frames.
 */
QColor windowFrameColor()
{
    return QGuiApplication::palette().window().color().darker(108);
}

/*!
 * \brief Returns the color used to draw instructions.
 */
QColor instructionTextColor()
{
    const auto baseColor = QGuiApplication::palette().base().color();
    return (baseColor.value() > 204 && baseColor.saturation() < 63) ? QColor(0x00, 0x33, 0x99) : QGuiApplication::palette().text().color();
}

# endif

/*!
 * \brief Returns the stylesheet for dialogs and other windows used in my applications.
 */
const QString &dialogStyle()
{
# ifdef Q_OS_WIN32
    static const auto style = QStringLiteral("#mainWidget { color: palette(text); background-color: palette(base); border: none; }"
                                             "#bottomWidget { background-color: palette(window); color: palette(window-text); border-top: 1px solid %1; }"
                                             "QMessageBox QLabel, QInputDialog QLabel, *[classNames~=\"heading\"] { font-size: 12pt; color: %2; }"
                                             "*[classNames~=\"input-invalid\"] { color: red; }").arg(
                                   windowFrameColor().name(), instructionTextColor().name());
# else
    static const auto style = QStringLiteral("*[classNames~=\"heading\"] { font-weight: bold; }"
                                             "*[classNames~=\"input-invalid\"] { color: red; }");
# endif
    return style;
}

/*!
 * \brief Makes \a widget a heading.
 */
void makeHeading(QWidget *widget)
{
    widget->setProperty("classNames", widget->property("classNames").toStringList() << QStringLiteral("heading"));
}

/*!
 * \brief Updates the widget style.
 * \remarks Useful when dynamic properties are used in the stylesheet because
 *          the widget style does not update automatically when a property changes.
 */
void updateStyle(QWidget *widget)
{
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

#endif

} // namespace Dialogs

