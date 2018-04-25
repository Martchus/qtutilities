#include "./dbusnotification.h"
#include "notificationsinterface.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusPendingReply>
#include <QImage>

#include <map>

using namespace std;

namespace MiscUtils {

/*!
 * \class DBusNotification
 * \brief The DBusNotification class emits D-Bus notifications.
 *
 * D-Bus notifications are only available if the library has been compiled with
 * support for it by specifying
 * CMake option `DBUS_NOTIFICATIONS`. If support is available, the macro
 * `QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS`
 * is defined.
 *
 * **Usage**
 *
 * First create a new instance. The constructor allows to set basic parameters.
 * To set more parameters, use
 * setter methods. Call show() to actually show the notification. This method
 * can also be used to update
 * the currently shown notification (it will not be updated automatically by
 * just using the setter methods).
 *
 * \sa https://developer.gnome.org/notification-spec
 */

/// \cond
static std::map<uint, DBusNotification *> pendingNotifications;
OrgFreedesktopNotificationsInterface *DBusNotification::s_dbusInterface = nullptr;
/// \endcond

/*!
 * \brief The SwappedImage struct represents RGB-interved version of the image specified on construction.
 */
struct SwappedImage : public QImage {
    SwappedImage(const QImage &image);
};

inline SwappedImage::SwappedImage(const QImage &image)
    : QImage(image.rgbSwapped())
{
}

/*!
 * \brief The NotificationImage struct is a raw data image format.
 *
 * It describes the width, height, rowstride, has alpha, bits per sample, channels and image data respectively.
 */
struct NotificationImage {
    NotificationImage();
    NotificationImage(const QVariant &imageData);
    NotificationImage(SwappedImage image);
    QImage toQImage() const;
    QVariant toDBusArgument() const;

    qint32 width;
    qint32 height;
    qint32 rowstride;
    bool hasAlpha;
    qint32 channels;
    qint32 bitsPerSample;
    QByteArray data;
    bool isValid;

private:
    NotificationImage(const QImage &image);
};

QDBusArgument &operator<<(QDBusArgument &argument, const NotificationImage &img)
{
    argument.beginStructure();
    argument << img.width;
    argument << img.height;
    argument << img.rowstride;
    argument << img.hasAlpha;
    argument << img.bitsPerSample;
    argument << img.channels;
    argument << img.data;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, NotificationImage &img)
{
    argument.beginStructure();
    argument >> img.width;
    argument >> img.height;
    argument >> img.rowstride;
    argument >> img.hasAlpha;
    argument >> img.bitsPerSample;
    argument >> img.channels;
    argument >> img.data;
    argument.endStructure();
    return argument;
}

inline NotificationImage::NotificationImage()
    : isValid(false)
{
}

inline NotificationImage::NotificationImage(const QVariant &imageData)
    : isValid(imageData.canConvert<QDBusArgument>())
{
    if (isValid) {
        imageData.value<QDBusArgument>() >> *this;
    }
}

NotificationImage::NotificationImage(SwappedImage image)
    : NotificationImage(static_cast<const QImage &>(image))
{
}

inline NotificationImage::NotificationImage(const QImage &image)
    : width(image.width())
    , height(image.height())
    , rowstride(image.bytesPerLine())
    , hasAlpha(image.hasAlphaChannel())
    , channels(image.isGrayscale() ? 1 : hasAlpha ? 4 : 3)
    , bitsPerSample(image.depth() / channels)
    , data(reinterpret_cast<const char *>(image.bits()), image.byteCount())
    , isValid(!image.isNull())
{
}

inline QImage NotificationImage::toQImage() const
{
    return isValid ? QImage(reinterpret_cast<const uchar *>(data.constData()), width, height, hasAlpha ? QImage::Format_ARGB32 : QImage::Format_RGB32)
                         .rgbSwapped()
                   : QImage();
}

inline QVariant NotificationImage::toDBusArgument() const
{
    QDBusArgument arg;
    return QVariant::fromValue(isValid ? arg << *this : arg);
}

} // namespace MiscUtils

Q_DECLARE_METATYPE(MiscUtils::NotificationImage);

namespace MiscUtils {

/*!
 * \brief Creates a new notification (which is *not* shown instantly).
 */
DBusNotification::DBusNotification(const QString &title, NotificationIcon icon, int timeout, QObject *parent)
    : QObject(parent)
    , m_id(0)
    , m_watcher(nullptr)
    , m_title(title)
    , m_timeout(timeout)
{
    initInterface();
    setIcon(icon);
}

/*!
 * \brief Creates a new notification (which is *not* shown instantly).
 */
DBusNotification::DBusNotification(const QString &title, const QString &icon, int timeout, QObject *parent)
    : QObject(parent)
    , m_id(0)
    , m_watcher(nullptr)
    , m_title(title)
    , m_icon(icon)
    , m_timeout(timeout)
{
    initInterface();
}

/*!
 * \brief Initializes the static interface object if not done yet.
 */
void DBusNotification::initInterface()
{
    if (!s_dbusInterface) {
        s_dbusInterface = new OrgFreedesktopNotificationsInterface(
            QStringLiteral("org.freedesktop.Notifications"), QStringLiteral("/org/freedesktop/Notifications"), QDBusConnection::sessionBus());
        connect(s_dbusInterface, &OrgFreedesktopNotificationsInterface::ActionInvoked, &DBusNotification::handleActionInvoked);
        connect(s_dbusInterface, &OrgFreedesktopNotificationsInterface::NotificationClosed, &DBusNotification::handleNotificationClosed);
    }
}

/*!
 * \brief Closes the notification if still shown and delete the object.
 */
DBusNotification::~DBusNotification()
{
    auto i = pendingNotifications.find(m_id);
    if (i != pendingNotifications.end()) {
        pendingNotifications.erase(i);
    }
    hide();
}

/*!
 * \brief Returns whether the notification D-Bus daemon is running.
 */
bool DBusNotification::isAvailable()
{
    initInterface();
    return s_dbusInterface->isValid();
}

/*!
 * \brief Sets the icon to one of the pre-defined notification icons.
 */
void DBusNotification::setIcon(NotificationIcon icon)
{
    switch (icon) {
    case NotificationIcon::Information:
        m_icon = QStringLiteral("dialog-information");
        break;
    case NotificationIcon::Warning:
        m_icon = QStringLiteral("dialog-warning");
        break;
    case NotificationIcon::Critical:
        m_icon = QStringLiteral("dialog-critical");
        break;
    default:;
    }
}

/*!
 * \brief Returns the image.
 * \sa setImage() for more details
 */
const QImage DBusNotification::image() const
{
    return NotificationImage(hint(QStringLiteral("image-data"), QStringLiteral("image_data"))).toQImage();
}

/*!
 * \brief Sets the image.
 * \remarks
 * \a image is a raw data image format which describes the width, height, rowstride,
 * has alpha, bits per sample, channels and image data respectively.
 */
void DBusNotification::setImage(const QImage &image)
{
    m_hints[QStringLiteral("image-data")] = NotificationImage(SwappedImage(image)).toDBusArgument();
}

/*!
 * \brief Makes the notification object delete itself when the notification has
 * been closed or an error occured.
 */
void DBusNotification::deleteOnCloseOrError()
{
    connect(this, &DBusNotification::closed, this, &DBusNotification::deleteLater);
    connect(this, &DBusNotification::error, this, &DBusNotification::deleteLater);
}

/*!
 * \brief Shows the notification.
 * \remarks If called when a previous notification is still shown, the previous
 * notification is updated.
 * \returns Returns false is the D-Bus daemon isn't reachable and true
 * otherwise.
 */
bool DBusNotification::show()
{
    if (!s_dbusInterface->isValid()) {
        emit error();
        return false;
    }

    delete m_watcher;
    m_watcher = new QDBusPendingCallWatcher(
        s_dbusInterface->Notify(QCoreApplication::applicationName(), m_id, m_icon, m_title, m_msg, m_actions, m_hints, m_timeout), this);
    connect(m_watcher, &QDBusPendingCallWatcher::finished, this, &DBusNotification::handleNotifyResult);
    return true;
}

/*!
 * \brief Updates the message and shows/updates the notification.
 * \remarks If called when a previous notification is still shown, the previous
 * notification is updated.
 * \returns Returns false is the D-Bus daemon isn't reachable and true
 * otherwise. The message is updated in any case.
 */
bool DBusNotification::show(const QString &message)
{
    m_msg = message;
    return show();
}

/*!
 * \brief Updates the message and shows/updates the notification.
 * \remarks
 * - If called when a previous notification is still shown, the previous
 * notification is updated. In this
 *   case the specified \a line will be appended to the current message.
 * - If called when no previous notification is still shown, the previous
 * message is completely replaced
 *   by \a line and shown as a new notification.
 * \returns Returns false is the D-Bus daemon isn't reachable and true
 * otherwise. The message is updated in any case.
 */
bool DBusNotification::update(const QString &line)
{
    if (!isVisible() || m_msg.isEmpty()) {
        m_msg = line;
    } else {
        if (!m_msg.startsWith(QStringLiteral("•"))) {
            m_msg.insert(0, QStringLiteral("• "));
        }
        m_msg.append(QStringLiteral("\n• "));
        m_msg.append(line);
    }
    return show();
}

bool DBusNotification::queryCapabilities(const std::function<void(Capabilities &&capabilities)> &callback)
{
    // ensure DBus-interface is initialized and valid
    initInterface();
    if (!s_dbusInterface->isValid()) {
        return false;
    }

    // invoke GetCapabilities() and pass the return value to the callback when available
    const auto *const watcher = new QDBusPendingCallWatcher(s_dbusInterface->GetCapabilities());
    connect(watcher, &QDBusPendingCallWatcher::finished, [&callback](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        const QDBusPendingReply<QStringList> returnValue(*watcher);
        if (returnValue.isError()) {
            callback(Capabilities());
        } else {
            callback(Capabilities(move(returnValue.value())));
        }
    });
    return true;
}

/*!
 * \brief Hides the notification (if still visible).
 * \remarks On success, the signal closed() is emitted with the reason
 * NotificationCloseReason::Manually.
 * \todo Add return value in v6.
 */
void DBusNotification::hide()
{
    if (m_id) {
        s_dbusInterface->CloseNotification(m_id);
    }
}

/*!
 * \brief Handles the results of the Notify D-Bus call.
 */
void DBusNotification::handleNotifyResult(QDBusPendingCallWatcher *watcher)
{
    if (watcher != m_watcher) {
        return;
    }

    watcher->deleteLater();
    m_watcher = nullptr;

    QDBusPendingReply<uint> returnValue = *watcher;
    if (returnValue.isError()) {
        deleteLater();
        emit error();
    } else {
        pendingNotifications[m_id = returnValue.argumentAt<0>()] = this;
        emit shown();
    }
}

/*!
 * \brief Handles the NotificationClosed D-Bus signal.
 */
void DBusNotification::handleNotificationClosed(uint id, uint reason)
{
    auto i = pendingNotifications.find(id);
    if (i != pendingNotifications.end()) {
        DBusNotification *notification = i->second;
        notification->m_id = 0;
        emit notification->closed(reason >= 1 && reason <= 3 ? static_cast<NotificationCloseReason>(reason) : NotificationCloseReason::Undefined);
        pendingNotifications.erase(i);
    }
}

/*!
 * \brief Handles the ActionInvoked D-Bus signal.
 */
void DBusNotification::handleActionInvoked(uint id, const QString &action)
{
    auto i = pendingNotifications.find(id);
    if (i != pendingNotifications.end()) {
        DBusNotification *notification = i->second;
        emit notification->actionInvoked(action);
        // Plasma 5 also closes the notification but doesn't emit the
        // NotificationClose signal
        // -> just consider the notification closed
        emit notification->closed(NotificationCloseReason::ActionInvoked);
        notification->m_id = 0;
        pendingNotifications.erase(i);
        // however, lxqt-notificationd does not close the notification
        // -> close manually for consistent behaviour
        s_dbusInterface->CloseNotification(i->first);
    }
}

/*!
 * \fn DBusNotification::message()
 * \brief Returns the assigned message.
 * \sa setMessage() for more details.
 */

/*!
 * \fn DBusNotification::setMessage()
 * \brief Sets the message to be shown.
 * \remarks
 * - Might also be set via show() and update().
 * - Can contain the following HTML tags: `<b>`, `<i>`, `<u>`, `<a href="...">`
 * and `<img src="..." alt="..."/>`
 */

/*!
 * \fn DBusNotification::timeout()
 * \brief Returns the number of milliseconds the notification will be visible
 * after calling show().
 * \sa setTimeout() for more details.
 */

/*!
 * \fn DBusNotification::setTimeout()
 * \brief Sets the number of milliseconds the notification will be visible after
 * calling show().
 * \remarks
 * - Set to 0 for non-expiring notifications.
 * - Set to -1 to let the notification daemon decide.
 */

/*!
 * \fn DBusNotification::actions()
 * \brief Returns the assigned actions.
 * \sa setActions() for more details.
 */

/*!
 * \fn DBusNotification::setActions()
 * \brief Sets the list of available actions.
 * \remarks
 * The list consists of pairs of action IDs and action labels, eg.
 * `QStringList({QStringLiteral("first_id"), tr("First action"),
 * QStringLiteral("second_id"), tr("Second action"), ...})`
 * \sa actionInvoked() signal
 */

/*!
 * \fn DBusNotification::isVisible()
 * \brief Returns whether the notification is (still) visible.
 */
} // namespace MiscUtils
