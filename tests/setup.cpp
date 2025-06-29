#include "../setup/updater.h"

#ifdef QT_UTILITIES_SETUP_TOOLS_HAS_OPENSSL_CRYPTO
#include "../setup/verification.h"
#endif

#include "resources/config.h"

#include <c++utilities/application/argumentparser.h>
#include <c++utilities/conversion/conversionexception.h>
#include <c++utilities/conversion/stringconversion.h>

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QSettings>
#include <QTimer>

#include <QtTest/QtTest>

#include <c++utilities/tests/testutils.h>

using namespace QtUtilities;

class SetupTests : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testUpdateNotifier();
    void testVerification();

private:
    CppUtilities::TestApplication m_app;
    double m_timeoutFactor = 1.0;
};

void SetupTests::initTestCase()
{
    CppUtilities::applicationInfo.url = "https://github.com/Martchus/syncthingtray";
    CppUtilities::applicationInfo.version = "1.4.0";

    if (!qEnvironmentVariableIsSet(PROJECT_VARNAME_UPPER "_UPDATER_VERBOSE")) {
        qputenv(PROJECT_VARNAME_UPPER "_UPDATER_VERBOSE", "1");
    }

    if (const auto timeoutFactorEnv = qgetenv(PROJECT_VARNAME_UPPER "_TEST_TIMEOUT_FACTOR"); !timeoutFactorEnv.isEmpty()) {
        try {
            m_timeoutFactor = CppUtilities::stringToNumber<double>(timeoutFactorEnv.data());
            qDebug() << "using timeout factor: " << m_timeoutFactor;
        } catch (const CppUtilities::ConversionException &) {
            qDebug() << "ignoring invalid " PROJECT_VARNAME_UPPER "_TEST_TIMEOUT_FACTOR";
        }
    }
}

void SetupTests::testUpdateNotifier()
{
    auto settings = QSettings();
    auto networkAccessManager = QNetworkAccessManager();
    auto *const diskCache = new QNetworkDiskCache(&networkAccessManager);
    auto cacheDirectory = CppUtilities::testDirPath("setup");
    diskCache->setCacheDirectory(QString::fromLocal8Bit(cacheDirectory));
    networkAccessManager.setCache(diskCache);
    auto updateHandler = UpdateHandler(&settings, &networkAccessManager);
    auto &updateNotifier = *updateHandler.notifier();
    if (!updateNotifier.isSupported()) {
        qDebug() << "skipping: UpdateNotifier() is not supported";
        return;
    }
    if (qEnvironmentVariableIntValue(PROJECT_VARNAME_UPPER "_TEST_ONLINE")) {
        updateHandler.setCacheLoadControl(QNetworkRequest::AlwaysNetwork);
    } else {
        updateHandler.setCacheLoadControl(QNetworkRequest::AlwaysCache);
    }

    auto eventLoop = QEventLoop();
    auto timeout = QTimer();
    timeout.setInterval(static_cast<int>(5000 * m_timeoutFactor));
    timeout.setSingleShot(true);

    auto newVersionFromSignal = QString();
    updateNotifier.checkForUpdate();

    QObject::connect(&updateNotifier, &UpdateNotifier::inProgressChanged, &eventLoop, &QEventLoop::quit);
    QObject::connect(&updateNotifier, &UpdateNotifier::updateAvailable, &updateNotifier,
        [&newVersionFromSignal](const QString &version, const QString &additionalInfo) {
            Q_UNUSED(additionalInfo)
            newVersionFromSignal = version;
        });
    QObject::connect(&timeout, &QTimer::timeout, &eventLoop, &QEventLoop::quit);

    timeout.start();
    eventLoop.exec();
    timeout.stop();

    QCOMPARE(updateNotifier.error(), QString());
    qDebug() << "download URL: " << updateNotifier.downloadUrl();
    QCOMPARE(updateNotifier.downloadUrl().host(), QStringLiteral("github.com"));
    QVERIFY2(updateNotifier.downloadUrl().path().startsWith(QLatin1String("/Martchus/syncthingtray/releases/download/")),
        "download URL contains expected path");
    QCOMPARE(updateNotifier.signatureUrl().path(), updateNotifier.downloadUrl().path() + QStringLiteral(".sig"));
    QVERIFY2(updateNotifier.isUpdateAvailable(), "update considered available");
    QVERIFY2(!updateNotifier.isInProgress(), "not timed out");
    QVERIFY2(!updateNotifier.newVersion().isEmpty(), "new version assigned");
    QVERIFY2(!newVersionFromSignal.isEmpty(), "updateAvailable() was emitted with non-empty version");
    QCOMPARE(updateNotifier.newVersion(), newVersionFromSignal);
    QVERIFY2(!QVersionNumber::fromString(newVersionFromSignal).isNull(), "version is parsable");
}

void SetupTests::testVerification()
{
#ifdef QT_UTILITIES_SETUP_TOOLS_HAS_OPENSSL_CRYPTO
    const auto key = std::string_view(
        R"(-----BEGIN PUBLIC KEY-----
MIGbMBAGByqGSM49AgEGBSuBBAAjA4GGAAQAWJAn1E7ZE5Q6H69oaV5sqCIppJdg
4bXDan9dJv6GOg70/t7q2CvwcwUXhV4FvCZxCHo25+rWYINfqKU2Utul8koAx8tK
59ohfOzI63I+CC76GfX41uRGU0P5i6hS7o/hgBLiVXqT0FgS2BMfmnLMUvUjqnI2
YQM7C55/5BM5Vrblkow=
-----END PUBLIC KEY-----)");
    const auto signature = std::string_view(
        R"(-----BEGIN SIGNATURE-----
MIGIAkIB+LB01DduBFMVs7Ea2McD7/kXpP0XktDNR7WpVgkOn4+/ilR8b8lpO9dd
FGmxKj5UVr2GpcWX6I216PjaVL9tr5oCQgFMpvNjSgFQ/KFaE+0d+QCegr3V7Uz6
sWB0iGdPa+oXbRish7HoNCU/k0lD3ffXaf8ueC78Zme9NFO18Ol+NWXJDA==
-----END SIGNATURE-----)");

    // test with valid message
    auto message = std::string("test message");
    QCOMPARE(verifySignature(key, signature, message), QString());

    // manipulate message, now it is no longer supposed to match
    message[5] = '?';
    QCOMPARE(verifySignature(key, signature, message), QStringLiteral("incorrect signature"));
#endif
}

QTEST_MAIN(SetupTests)
#include "setup.moc"
