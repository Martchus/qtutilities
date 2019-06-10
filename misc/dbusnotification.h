#ifndef MISC_UTILS_NOTIFICATION_H
#define MISC_UTILS_NOTIFICATION_H

#include "../global.h"

#include <QObject>
#include <QSet>
#include <QVariantMap>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QDBusPendingCallWatcher)

class OrgFreedesktopNotificationsInterface;

namespace QtUtilities {

enum class NotificationIcon { NoIcon, Information, Warning, Critical };

enum class NotificationCloseReason { Undefined, Expired, Dismissed, Manually, ActionInvoked };

class QT_UTILITIES_EXPORT DBusNotification : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle)
    Q_PROPERTY(QString message READ message WRITE setMessage)
    Q_PROPERTY(QString icon READ icon WRITE setIcon)
    Q_PROPERTY(int timeout READ timeout WRITE setTimeout)
    Q_PROPERTY(QStringList actions READ actions WRITE setActions)
    Q_PROPERTY(bool visible READ isVisible)

public:
    class QT_UTILITIES_EXPORT Capabilities : public QSet<QString> {
    public:
        explicit Capabilities();
        explicit Capabilities(const QStringList &capabilities);
        bool isValid() const;
        bool supportsBody() const;
        bool supportsLinks() const;
        bool supportsMarkup() const;
        bool supportsImages() const;
        bool supportsIcon() const;
        bool supportsActions() const;
        bool supportsAnimatedIcon() const;
        bool supportsActionIcons() const;
        bool supportsSound() const;
        bool supportsPercistence() const;

    private:
        bool m_valid;
    };

    explicit DBusNotification(
        const QString &title, NotificationIcon icon = NotificationIcon::Information, int timeout = 10000, QObject *parent = nullptr);
    explicit DBusNotification(const QString &title, const QString &icon, int timeout = 10000, QObject *parent = nullptr);
    ~DBusNotification() override;

    static bool isAvailable();
    const QString &title() const;
    void setTitle(const QString &title);
    const QString &message() const;
    void setMessage(const QString &message);
    const QString &icon() const;
    void setIcon(const QString &icon);
    void setIcon(NotificationIcon icon);
    const QImage image() const;
    void setImage(const QImage &image);
    const QString imagePath() const;
    void setImagePath(const QString &imagePath);
    int timeout() const;
    void setTimeout(int timeout);
    const QStringList &actions() const;
    void setActions(const QStringList &actions);
    const QVariantMap &hints() const;
    QVariantMap &hints();
    QVariant hint(const QString &name) const;
    QVariant hint(const QString &name, const QString &fallbackNames...) const;
    bool isVisible() const;
    void deleteOnCloseOrError();
    static bool queryCapabilities(const std::function<void(Capabilities &&capabilities)> &callback);

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
    static OrgFreedesktopNotificationsInterface *s_dbusInterface;
};

inline DBusNotification::Capabilities::Capabilities()
    : m_valid(false)
{
}

inline DBusNotification::Capabilities::Capabilities(const QStringList &capabilities)
    : QSet<QString>(capabilities.toSet())
    , m_valid(true)
{
}

inline bool DBusNotification::Capabilities::isValid() const
{
    return m_valid;
}

inline bool DBusNotification::Capabilities::supportsBody() const
{
    return contains(QStringLiteral("body"));
}

inline bool DBusNotification::Capabilities::supportsLinks() const
{
    return contains(QStringLiteral("body-hyperlinks"));
}

inline bool DBusNotification::Capabilities::supportsMarkup() const
{
    return contains(QStringLiteral("body-markup"));
}

inline bool DBusNotification::Capabilities::supportsImages() const
{
    return contains(QStringLiteral("body-images"));
}

inline bool DBusNotification::Capabilities::supportsIcon() const
{
    return contains(QStringLiteral("icon-static")) || supportsAnimatedIcon();
}

inline bool DBusNotification::Capabilities::supportsActions() const
{
    return contains(QStringLiteral("actions"));
}

inline bool DBusNotification::Capabilities::supportsAnimatedIcon() const
{
    return contains(QStringLiteral("icon-multi"));
}

inline bool DBusNotification::Capabilities::supportsActionIcons() const
{
    return contains(QStringLiteral("action-icons"));
}

inline bool DBusNotification::Capabilities::supportsSound() const
{
    return contains(QStringLiteral("sound"));
}

inline bool DBusNotification::Capabilities::supportsPercistence() const
{
    return contains(QStringLiteral("persistence"));
}

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
 * The specified \a icon should be either an URI (file:// is the only URI schema
 * supported right now) or a name in an icon theme.
 */
inline void DBusNotification::setIcon(const QString &icon)
{
    m_icon = icon;
}

/*!
 * \brief Returns the hint with the specified \a name.
 */
inline QVariant DBusNotification::hint(const QString &name) const
{
    return m_hints[name];
}

/*!
 * \brief Returns the hint with the specified \a name. If no hint is present, the \a fallbackNames are tried in the specified order.
 */
inline QVariant DBusNotification::hint(const QString &name, const QString &fallbackNames...) const
{
    const auto variant(m_hints[name]);
    return variant.isNull() ? this->hint(fallbackNames) : variant;
}

/*!
 * \brief Returns the image path.
 * \sa setImagePath() for more details
 */
inline const QString DBusNotification::imagePath() const
{
    return hint(QStringLiteral("image-data"), QStringLiteral("image_path")).toString();
}

/*!
 * \brief Sets the image path.
 * \remarks
 * Alternative way to define the notification image; setImage() precedes.
 */
inline void DBusNotification::setImagePath(const QString &imagePath)
{
    m_hints[QStringLiteral("image-path")] = imagePath;
}

inline int DBusNotification::timeout() const
{
    return m_timeout;
}

inline void DBusNotification::setTimeout(int timeout)
{
    m_timeout = timeout;
}

/*!
 * \brief Returns the actions for the notification.
 *
 * The actions are a list of action IDs and action names. The ID is returned by
 * the actionInvoked() signal if an action is triggered. The action name is the
 * user-visible name of the notification.
 *
 * Example: { QStringLiteral("dismiss"), tr("Dismiss notification"),
 *            QStringLiteral("details"), tr("Show details") }
 */
inline const QStringList &DBusNotification::actions() const
{
    return m_actions;
}

/*!
 * \brief Sets the actions for the notification.
 * \sa see actions() for details and an example
 */
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
} // namespace QtUtilities

#endif // MISC_UTILS_NOTIFICATION_H
