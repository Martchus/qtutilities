#include "./desktoputils.h"

#include <QDesktopServices>
#include <QUrl>
#if (QT_VERSION >= QT_VERSION_CHECK(6, 5, 0))
#include <QGuiApplication>
#include <QStyleHints>
#endif
#ifdef Q_OS_WIN32
#include <QFileInfo>
#endif

namespace QtUtilities {

/*!
 * \brief Shows the specified file or directory using the default file browser.
 * \remarks
 * - The specified \a path must *not* be specified as URL. (The conversion to a URL suitable for
 *   QDesktopServices::openUrl() is the whole purpose of this function).
 * - The Qt documentation suggests to use
 *   `QDesktopServices::openUrl(QUrl("file:///C:/Documents and Settings/All Users/Desktop", QUrl::TolerantMode));`
 *   under Windows. However, that does not work if the path contains a '#'. It is also better to use
 *   QUrl::DecodedMode to prevent QUrl from interpreting any of the paths characters in a special way.
 */
bool openLocalFileOrDir(const QString &path)
{
    QUrl url(QStringLiteral("file://"));

#ifdef Q_OS_WIN32
    // replace backslashes with regular slashes
    QString tmp(path);
    tmp.replace(QChar('\\'), QChar('/'));

    // add a slash before the drive letter of an absolute path
    if (QFileInfo(path).isAbsolute()) {
        tmp = QStringLiteral("/") + tmp;
    }
    url.setPath(tmp, QUrl::DecodedMode);

#else
    url.setPath(path, QUrl::DecodedMode);
#endif
    return QDesktopServices::openUrl(url);
}

/*!
 * \brief Returns whether \a palette is dark.
 */
bool isPaletteDark(const QPalette &palette)
{
    return palette.color(QPalette::WindowText).lightness() > palette.color(QPalette::Window).lightness();
}

/*!
 * \brief Returns whether dark mode is enabled.
 * \remarks Whether dark mode is enabled can only be determined on Qt 6.5 or newer and only on certain
 *          platforms. If it cannot be determined, std::nullopt is returned.
 */
std::optional<bool> isDarkModeEnabled()
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 5, 0))
    if (const auto *const styleHints = QGuiApplication::styleHints()) {
        const auto colorScheme = styleHints->colorScheme();
        if (colorScheme != Qt::ColorScheme::Unknown) {
            return colorScheme == Qt::ColorScheme::Dark;
        }
    }
#endif
    return std::nullopt;
}

} // namespace QtUtilities
