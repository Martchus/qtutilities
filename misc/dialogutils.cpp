#include "./dialogutils.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#if defined(QT_UTILITIES_GUI_QTWIDGETS) || defined(QT_UTILITIES_GUI_QTQUICK)
#include <QGuiApplication>
#include <QPalette>
#endif

#if defined(QT_UTILITIES_GUI_QTWIDGETS)
#include <QApplication>
#include <QCursor>
#if (QT_VERSION < QT_VERSION_CHECK(5, 10, 0))
#include <QDesktopWidget>
#endif
#include <QScreen>
#include <QStyle>
#include <QWidget>
#endif

namespace QtUtilities {

/*!
 * \brief Generates the window title string for the specified \a documentStatus
 *        and \a documentPath.
 */
QString generateWindowTitle(DocumentStatus documentStatus, const QString &documentPath)
{
    switch (documentStatus) {
    case DocumentStatus::Saved:
        if (documentPath.isEmpty()) {
            return QCoreApplication::translate("Utilities::windowTitle", "Unsaved - %1").arg(QCoreApplication::applicationName());
        } else {
            const QFileInfo file(documentPath);
            return QCoreApplication::translate("Utilities::windowTitle", "%1 - %2 - %3")
                .arg(file.fileName(), file.dir().path(), QCoreApplication::applicationName());
        }
    case DocumentStatus::Unsaved:
        if (documentPath.isEmpty()) {
            return QCoreApplication::translate("Utilities::windowTitle", "*Unsaved - %1").arg(QCoreApplication::applicationName());
        } else {
            const QFileInfo file(documentPath);
            return QCoreApplication::translate("Utilities::windowTitle", "*%1 - %2 - %3")
                .arg(file.fileName(), file.dir().path(), QCoreApplication::applicationName());
        }
    case DocumentStatus::NoDocument:
        return QCoreApplication::applicationName();
    default:
        return QString(); // to suppress warning: "control reaches end of non-void
        // function"
    }
}

#if defined(QT_UTILITIES_GUI_QTWIDGETS) || defined(QT_UTILITIES_GUI_QTQUICK)

#ifdef Q_OS_WIN32

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

#endif

/*!
 * \brief Returns the stylesheet for dialogs and other windows used in my
 * applications.
 */
const QString &dialogStyle()
{
#ifdef Q_OS_WIN32
    static const auto style = QStringLiteral("#mainWidget { color: palette(text); background-color: "
                                             "palette(base); border: none; }"
                                             "#bottomWidget { background-color: palette(window); "
                                             "color: palette(window-text); border-top: 1px solid %1; }"
                                             "QMessageBox QLabel, QInputDialog QLabel, "
                                             "*[classNames~=\"heading\"] { font-size: 12pt; color: %2; "
                                             "}"
                                             "*[classNames~=\"input-invalid\"] { color: red; }")
                                  .arg(windowFrameColor().name(), instructionTextColor().name());
#else
    static const auto style = QStringLiteral("*[classNames~=\"heading\"] { font-weight: bold; }"
                                             "*[classNames~=\"input-invalid\"] { color: red; }");
#endif
    return style;
}

#ifdef QT_UTILITIES_GUI_QTWIDGETS

QRect availableScreenGeometryAtPoint(const QPoint &point)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    QScreen *const screen = QGuiApplication::screenAt(point);
    if (!screen) {
        return QRect();
    }
    return screen->availableGeometry();
#else
    return QApplication::desktop()->availableGeometry(point);
#endif
}

/*!
 * \brief Moves the specified \a widget in the middle of the (available) screen
 * area or \a parent if specified.
 *
 * The screen containing the current cursor position is used unless \a position
 * is specified.
 */
void centerWidget(QWidget *widget, const QWidget *parent, const QPoint *position)
{
    widget->setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, widget->size(),
        parent ? parent->geometry() : availableScreenGeometryAtPoint(position ? *position : QCursor::pos())));
}

/*!
 * \brief Moves the specified \a widget to the corner which is closest to the
 * current cursor position or \a position if specified.
 *
 * If there are multiple screens available, the screen where the cursor currently
 * is located is chosen.
 */
void cornerWidget(QWidget *widget, const QPoint *position)
{
    const QPoint cursorPos(position ? *position : QCursor::pos());
    const QRect availableGeometry(availableScreenGeometryAtPoint(cursorPos));
    const Qt::Alignment alignment
        = (cursorPos.x() - availableGeometry.left() < availableGeometry.right() - cursorPos.x() ? Qt::AlignLeft : Qt::AlignRight)
        | (cursorPos.y() - availableGeometry.top() < availableGeometry.bottom() - cursorPos.y() ? Qt::AlignTop : Qt::AlignBottom);
    widget->setGeometry(QStyle::alignedRect(Qt::LeftToRight, alignment, widget->size(), availableGeometry));
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
 *          the widget style does not update automatically when a property
 * changes.
 */
void updateStyle(QWidget *widget)
{
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

#endif

#endif

} // namespace QtUtilities
