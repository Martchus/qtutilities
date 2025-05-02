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

#if defined(Q_OS_ANDROID) && (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
#define QT_UTILITIES_OPEN_VIA_ACTIVITY
#include <QCoreApplication>
#include <QJniEnvironment>
#include <QJniObject>
#endif

namespace QtUtilities {

/*!
 * \brief Shows the specified file or directory using the default file browser.
 * \remarks
 * - The specified \a path must *not* be specified as URL. (The conversion to a URL suitable for
 *   QDesktopServices::openUrl() is the main purpose of this function).
 * - The Qt documentation suggests to use
 *   `QDesktopServices::openUrl(QUrl("file:///C:/Documents and Settings/All Users/Desktop", QUrl::TolerantMode));`
 *   under Windows. However, that does not work if the path contains a '#'. It is also better to use
 *   QUrl::DecodedMode to prevent QUrl from interpreting any of the paths characters in a special way.
 * - Tries to open the path via the activity method `openPath()` under Android so that method can be implemented
 *   to override the behavior if needed. The method must take a string and return a boolean. If it does not exist
 *   the usual QDesktopServices::openUrl() is done.
 */
bool openLocalFileOrDir(const QString &path)
{
#ifdef QT_UTILITIES_OPEN_VIA_ACTIVITY
    if (const auto context = QNativeInterface::QAndroidApplication::context(); context.isValid()) {
        auto env = QJniEnvironment();
        if (auto openPathMethod = env.findMethod(context.objectClass(), "openPath", "(Ljava/lang/String;)Z")) {
            return env->CallBooleanMethod(context.object(), openPathMethod, QJniObject::fromString(path).object()) == JNI_TRUE;
        }
    }
#endif

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

/*!
 * \brief Invokes the specified callback when the color scheme changed.
 *
 * The first argument of the callback will be whether the color scheme is dark now (Qt::ColorScheme::Dark).
 * The callback is invoked immediately by this function unless \a invokeImmediately is set to false.
 *
 * \remarks Whether dark mode is enabled can only be determined on Qt 6.5 or newer and only on certain
 *          platforms. If it cannot be determined the \a darkModeChangedCallback is never invoked.
 */
QMetaObject::Connection onDarkModeChanged(std::function<void(bool)> &&darkModeChangedCallback, QObject *context, bool invokeImmediately)
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 5, 0))
    if (const auto *const styleHints = QGuiApplication::styleHints()) {
        if (invokeImmediately) {
            darkModeChangedCallback(styleHints->colorScheme() == Qt::ColorScheme::Dark);
        }
        return QObject::connect(styleHints, &QStyleHints::colorSchemeChanged, context,
            [handler = std::move(darkModeChangedCallback)](Qt::ColorScheme colorScheme) { return handler(colorScheme == Qt::ColorScheme::Dark); });
    }
#else
    Q_UNUSED(darkModeChangedCallback)
    Q_UNUSED(context)
    Q_UNUSED(invokeImmediately)
#endif
    return QMetaObject::Connection();
}

} // namespace QtUtilities
