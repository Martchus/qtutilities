#ifndef MISC_UTILS_NOTIFICATION_H
#define MISC_UTILS_NOTIFICATION_H

#include "../global.h"

#include <QObject>
#include <QVariantMap>

QT_FORWARD_DECLARE_CLASS(QDBusPendingCallWatcher)

class OrgFreedesktopNotificationsInterface;

namespace MiscUtils {

enum class NotificationIcon
{
    NoIcon,
    Information,
    Warning,
    Critical
};

enum class NotificationCloseReason
{
    Undefined,
    Expired,
    Dismissed,
    Manually,
    ActionInvoked
};

class QT_UTILITIES_EXPORT DBusNotification : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle)
    Q_PROPERTY(QString message READ message WRITE setMessage)
    Q_PROPERTY(QString icon READ icon WRITE setIcon)
    Q_PROPERTY(int timeout READ timeout WRITE setTimeout)
    Q_PROPERTY(QStringList actions READ actions WRITE setActions)
    Q_PROPERTY(bool visible READ isVisible)

public:
    explicit DBusNotification(const QString &title, NotificationIcon icon = NotificationIcon::Information, int timeout = 10000, QObject *parent = nullptr);
    explicit DBusNotification(const QString &title, const QString &icon, int timeout = 10000, QObject *parent = nullptr);
    ~DBusNotification();

    static bool isAvailable();
    const QString &title() const;
    void setTitle(const QString &title);
    const QString &message() const;
    void setMessage(const QString &message);
    const QString &icon() const;
    void setIcon(const QString &icon);
    void setIcon(NotificationIcon icon);
    int timeout() const;
    void setTimeout(int timeout);
    const QStringList &actions() const;
    void setActions(const QStringList &actions);
    const QVariantMap &hints() const;
    QVariantMap &hints();
    bool isVisible() const;
    void deleteOnCloseOrError();

public Q_SLOTS:
    bool show();
    bool show(const QString &message);
    bool update(const QString &line);
    void hide();

Q_SIGNALS:
    /// \brief Emitted when the notification could be shown successful.
    void shown();
    /// \brief Emitted when the notification couldn't be shown.
    void error();
    /// \brief Emitted when the notification has been closed.
    void closed(NotificationCloseReason reason);
    /// \brief Emitted when \a action has been invoked.
    void actionInvoked(const QString &action);

private Q_SLOTS:
    void handleNotifyResult(QDBusPendingCallWatcher *);
    static void handleNotificationClosed(uint id, uint reason);
    static void handleActionInvoked(uint id, const QString &action);

private:
    static void initInterface();

    uint m_id;
    QDBusPendingCallWatcher *m_watcher;
    QString m_title;
    QString m_msg;
    QString m_icon;
    int m_timeout;
    QStringList m_actions;
    QVariantMap m_hints;
    static OrgFreedesktopNotificationsInterface *m_dbusInterface;
};

inline const QString &DBusNotification::title() const
{
    return m_title;
}

inline void DBusNotification::setTitle(const QString &title)
{
    m_title = title;
}

inline const QString &DBusNotification::message() const
{
    return m_msg;
}

inline void DBusNotification::setMessage(const QString &message)
{
    m_msg = message;
}

/*!
 * \brief Returns the icon name.
 * \sa setIcon() for more details
 */
inline const QString &DBusNotification::icon() const
{
    return m_icon;
}

/*!
 * \brief Sets the icon name.
 * \remarks
 * The specified \a icon should be either an URI (file:// is the only URI schema supported
 * right now) or a name in an icon theme.
 */
inline void DBusNotification::setIcon(const QString &icon)
{
    m_icon = icon;
}

inline int DBusNotification::timeout() const
{
    return m_timeout;
}

inline void DBusNotification::setTimeout(int timeout)
{
    m_timeout = timeout;
}

inline const QStringList &DBusNotification::actions() const
{
    return m_actions;
}

inline void DBusNotification::setActions(const QStringList &actions)
{
    m_actions = actions;
}

inline const QVariantMap &DBusNotification::hints() const
{
    return m_hints;
}

inline QVariantMap &DBusNotification::hints()
{
    return m_hints;
}

inline bool DBusNotification::isVisible() const
{
    return m_id != 0;
}

}

#endif // MISC_UTILS_NOTIFICATION_H
