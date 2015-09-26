#include "./dialogutils.h"

#ifdef GUI_NONE
#include <QCoreApplication>
#else
#include <QGuiApplication>
#include <QPalette>
#include <QWidget>
#endif
#include <QFileInfo>
#include <QDir>

namespace Dialogs {

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

#ifdef Q_OS_WIN32
QColor windowFrameColor()
{
    return QGuiApplication::palette().window().color().darker(108);
}

QColor instructionTextColor()
{
    const auto baseColor = QGuiApplication::palette().base().color();
    return (baseColor.value() > 204 && baseColor.saturation() < 63) ? QColor(0x00, 0x33, 0x99) : QGuiApplication::palette().text().color();
}

#endif

const QString &dialogStyle()
{
#ifdef Q_OS_WIN32
    static const auto style = QStringLiteral(" #mainWidget { color: palette(text); background-color: palette(base); border: none; }"
                                             " #bottomWidget { background-color: palette(window); color: palette(window-text); border-top: 1px solid %1; }"
                                             " QMessageBox QLabel, QInputDialog QLabel, *[classNames~=\"heading\"] { font-size: 12pt; color: %2; } ").arg(
                                   windowFrameColor().name(), instructionTextColor().name());
#else
    static const auto style = QStringLiteral(" *[classNames~=\"heading\"] { font-weight: bold; } ");
#endif
    return style;
}

void makeHeading(QWidget *widget)
{
    widget->setProperty("classNames", widget->property("classNames").toStringList() << QStringLiteral("heading"));
}

#endif

} // namespace Dialogs

