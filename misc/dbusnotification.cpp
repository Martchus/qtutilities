#include "./dbusnotification.h"
#include "notificationsinterface.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusPendingReply>

#include <map>

using namespace std;

namespace MiscUtils {

/*!
 * \class DBusNotification
 * \brief The DBusNotification class emits D-Bus notifications.
 *
 * D-Bus notifications are only available if the library has been compiled with support for it by specifying
 * CMake option `DBUS_NOTIFICATIONS`. If support is available, the macro `QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS`
 * is defined.
 *
 * **Usage**
 *
 * First create a new instance. The constructor allows to set basic parameters. To set more parameters, use
 * setter methods. Call show() to actually show the notification. This method can also be used to update
 * the currently shown notification (it will not be updated automatically by just using the setter methods).
 *
 * \sa https://developer.gnome.org/notification-spec
 */

/// \cond
static std::map<uint, DBusNotification *> pendingNotifications;
OrgFreedesktopNotificationsInterface *DBusNotification::m_dbusInterface = nullptr;
/// \endcond

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
    if (!m_dbusInterface) {
        m_dbusInterface = new OrgFreedesktopNotificationsInterface(
            QStringLiteral("org.freedesktop.Notifications"), QStringLiteral("/org/freedesktop/Notifications"), QDBusConnection::sessionBus());
        connect(m_dbusInterface, &OrgFreedesktopNotificationsInterface::ActionInvoked, &DBusNotification::handleActionInvoked);
        connect(m_dbusInterface, &OrgFreedesktopNotificationsInterface::NotificationClosed, &DBusNotification::handleNotificationClosed);
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
    return m_dbusInterface->isValid();
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
 * \brief Makes the notification object delete itself when the notification has been closed or an error occured.
 */
void DBusNotification::deleteOnCloseOrError()
{
    connect(this, &DBusNotification::closed, this, &DBusNotification::deleteLater);
    connect(this, &DBusNotification::error, this, &DBusNotification::deleteLater);
}

/*!
 * \brief Shows the notification.
 * \remarks If called when a previous notification is still shown, the previous notification is updated.
 * \returns Returns false is the D-Bus daemon isn't reachable and true otherwise.
 */
bool DBusNotification::show()
{
    if (!m_dbusInterface->isValid()) {
        emit error();
        return false;
    }

    delete m_watcher;
    m_watcher = new QDBusPendingCallWatcher(
        m_dbusInterface->Notify(QCoreApplication::applicationName(), m_id, m_icon, m_title, m_msg, m_actions, m_hints, m_timeout), this);
    connect(m_watcher, &QDBusPendingCallWatcher::finished, this, &DBusNotification::handleNotifyResult);
    return true;
}

/*!
 * \brief Updates the message and shows/updates the notification.
 * \remarks If called when a previous notification is still shown, the previous notification is updated.
 * \returns Returns false is the D-Bus daemon isn't reachable and true otherwise. The message is updated in any case.
 */
bool DBusNotification::show(const QString &message)
{
    m_msg = message;
    return show();
}

/*!
 * \brief Updates the message and shows/updates the notification.
 * \remarks
 * - If called when a previous notification is still shown, the previous notification is updated. In this
 *   case the specified \a line will be appended to the current message.
 * - If called when no previous notification is still shown, the previous message is completely replaced
 *   by \a line and shown as a new notification.
 * \returns Returns false is the D-Bus daemon isn't reachable and true otherwise. The message is updated in any case.
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

/*!
 * \brief Hides the notification (if still visible).
 * \remarks On success, the signal closed() is emitted with the reason NotificationCloseReason::Manually.
 */
void DBusNotification::hide()
{
    if (m_id) {
        m_dbusInterface->CloseNotification(m_id);
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
        // Plasma 5 also closes the notification but doesn't emit the NotificationClose signal
        // -> just consider the notification closed
        emit notification->closed(NotificationCloseReason::ActionInvoked);
        notification->m_id = 0;
        pendingNotifications.erase(i);
        // however, lxqt-notificationd does not close the notification
        // -> close manually for consistent behaviour
        m_dbusInterface->CloseNotification(i->first);
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
 * - Can contain the following HTML tags: `<b>`, `<i>`, `<u>`, `<a href="...">` and `<img src="..." alt="..."/>`
 */

/*!
 * \fn DBusNotification::timeout()
 * \brief Returns the number of milliseconds the notification will be visible after calling show().
 * \sa setTimeout() for more details.
 */

/*!
 * \fn DBusNotification::setTimeout()
 * \brief Sets the number of milliseconds the notification will be visible after calling show().
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
 * `QStringList({QStringLiteral("first_id"), tr("First action"), QStringLiteral("second_id"), tr("Second action"), ...})`
 * \sa actionInvoked() signal
 */

/*!
 * \fn DBusNotification::isVisible()
 * \brief Returns whether the notification is (still) visible.
 */
}
