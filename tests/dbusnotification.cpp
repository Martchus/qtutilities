#include "../misc/dbusnotification.h"

#include "resources/config.h"

#include <QSignalSpy>
#include <QTest>

using namespace QtUtilities;

/*!
 * \brief The DBusNotificationTests class tests the DBusNotification class.
 */
class DBusNotificationTests : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void smokeTest();
    void semiAutomaticTest();
};

static void dummy(DBusNotification::Capabilities &&)
{
}
const static auto callback = std::function<void(DBusNotification::Capabilities &&)>(&dummy);

/*!
 * \brief Runs some basic functions of DBusNotification (c'tor, d'tor, some accessors, ...) but doesn't really check
 *        whether it works.
 * \remarks This test should pass regardless whether DBus-notifications are actually available. Hence it avoids any checks
 *          which depend on that and just checks whether certain function don't lead to crashes.
 */
void DBusNotificationTests::smokeTest()
{
    DBusNotification::isAvailable();
    DBusNotification::queryCapabilities(callback);
    DBusNotification n(QStringLiteral("Smoke test"), NotificationIcon::Warning, 100);
    QVERIFY2(!n.isVisible(), "not immediately visible");
    QCOMPARE(n.title(), QStringLiteral("Smoke test"));
    n.setApplicationName(QStringLiteral(APP_NAME " tests; " APP_VERSION));
    QCOMPARE(n.applicationName(), QStringLiteral(APP_NAME " tests; " APP_VERSION));
    n.show(QStringLiteral("Some message")); // will emit an error if not available
    n.isVisible();
    n.hide();
    QCOMPARE(n.message(), QStringLiteral("Some message"));
    n.update(QStringLiteral("Another message"));
    n.hide();
    if (n.isVisible()) {
        QSignalSpy closedSpy(&n, &DBusNotification::closed);
        closedSpy.wait();
    }
}

/*!
 * \brief Runs a semi-automatic test to verify whether DBusNotification works for real.
 * \remarks This test needs a daemon for D-Bus notifications running and requires manual user interaction. It is therefore
 * skipped unless an environment variable is set.
 */
void DBusNotificationTests::semiAutomaticTest()
{
    const auto envValue = qgetenv(PROJECT_VARNAME_UPPER "_ENABLE_SEMI_AUTOMATIC_NOTIFICATION_TESTS");
    auto envValueIsInt = false;
    if (envValue.isEmpty() || (envValue.toInt(&envValueIsInt) == 0 && envValueIsInt)) {
        QSKIP("Set the environment variable " PROJECT_VARNAME_UPPER "_ENABLE_SEMI_AUTOMATIC_NOTIFICATION_TESTS to run "
              "the semi-automatic D-Bus notification test.");
    }

    QVERIFY2(DBusNotification::isAvailable(), "D-Bus notifications are available");

    DBusNotification n(QStringLiteral("Semi-automatic test"), NotificationIcon::Information, 10000);
    QString clickedAction, error;
    const auto actionConnection
        = connect(&n, &DBusNotification::actionInvoked, [&clickedAction](const QString &actionName) { clickedAction = actionName; });
    const auto errorConnection
        = connect(&n, &DBusNotification::error, [&error]() { error = QStringLiteral("error occurred (TODO: pass an error message here)"); });
    n.setApplicationName(QStringLiteral(APP_NAME " tests; " APP_VERSION));
    n.show(QStringLiteral("Some message; will append more lines later"));
    for (auto i = 1; i <= 10; ++i) {
        n.update(QStringLiteral("Yet another line, should be displayed in the same notification as previous message (%1)").arg(i));
        QTest::qWait(100);
    }
    QCOMPARE(error, QString());
    n.setImage(QIcon::fromTheme(QStringLiteral("document-open")).pixmap(64).toImage());
    n.setTitle(n.title() + QStringLiteral(" - click action to continue"));
    n.setActions(QStringList({ QStringLiteral("fail"), QStringLiteral("Let test fail"), QStringLiteral("pass"), QStringLiteral("Let test pass") }));
    QSignalSpy actionInvokedSpy(&n, &DBusNotification::actionInvoked);
    n.update(QStringLiteral("Click on \"Let test pass\" to continue within 10 seconds"));
    actionInvokedSpy.wait(10000);
    QCOMPARE(clickedAction, QStringLiteral("pass"));
    QSignalSpy closedSpy(&n, &DBusNotification::closed);
    n.setTimeout(5000);
    n.setActions(QStringList());
    n.update(QStringLiteral("Waiting for message to close (will close automatically in 5 seconds)"));
    closedSpy.wait(8000);

    QCOMPARE(error, QString());
    disconnect(actionConnection);
    disconnect(errorConnection);
}

QTEST_MAIN(DBusNotificationTests)
#include "dbusnotification.moc"
