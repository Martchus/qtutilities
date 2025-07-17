#include "./updater.h"

#if defined(QT_UTILITIES_GUI_QTWIDGETS)
#include "../settingsdialog/optioncategory.h"
#endif

#include "resources/config.h"

#include <QSettings>
#include <QTimer>

#include <c++utilities/application/argumentparser.h>

#include <optional>

#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
#include <c++utilities/io/ansiescapecodes.h>
#include <c++utilities/io/archive.h>

#include <QCoreApplication>
#include <QDebug>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProcess>
#include <QRegularExpression>
#include <QStringBuilder>
#include <QVersionNumber>
#include <QtConcurrentRun>
#include <QtGlobal> // for QtProcessorDetection and QtSystemDetection keeping it Qt 5 compatible

#if defined(QT_UTILITIES_GUI_QTWIDGETS)
#include <QMessageBox>
#endif

#include <iostream>
#endif

#if defined(QT_UTILITIES_GUI_QTWIDGETS)
#include <QCoreApplication>
#include <QLabel>

#if defined(QT_UTILITIES_SETUP_TOOLS_ENABLED)
#include "ui_updateoptionpage.h"
#else
namespace QtUtilities {
namespace Ui {
class UpdateOptionPage {
public:
    void setupUi(QWidget *)
    {
    }
    void retranslateUi(QWidget *)
    {
    }
};
} // namespace Ui
} // namespace QtUtilities
#endif
#endif

#include "resources/config.h"

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
#define QT_UTILITIES_VERSION_SUFFIX QString()
#else
#define QT_UTILITIES_VERSION_SUFFIX QStringLiteral("-qt5")
#endif

#if defined(Q_OS_WINDOWS)
#define QT_UTILITIES_EXE_REGEX "\\.exe"
#else
#define QT_UTILITIES_EXE_REGEX ""
#endif

#if defined(Q_OS_WIN64)
#if defined(Q_PROCESSOR_X86_64)
#define QT_UTILITIES_DOWNLOAD_REGEX "-.*-x86_64-w64-mingw32"
#elif defined(Q_PROCESSOR_ARM_64)
#define QT_UTILITIES_DOWNLOAD_REGEX "-.*-aarch64-w64-mingw32"
#endif
#elif defined(Q_OS_WIN32)
#define QT_UTILITIES_DOWNLOAD_REGEX "-.*-i686-w64-mingw32"
#elif defined(__GNUC__) && defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#if defined(Q_PROCESSOR_X86_64)
#define QT_UTILITIES_DOWNLOAD_REGEX "-.*-x86_64-pc-linux-gnu"
#elif defined(Q_PROCESSOR_ARM_64)
#define QT_UTILITIES_DOWNLOAD_REGEX "-.*-aarch64-pc-linux-gnu"
#endif
#endif

namespace QtUtilities {

#if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
using VersionSuffixIndex = qsizetype;
#else
using VersionSuffixIndex = int;
#endif

#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
struct UpdateNotifierPrivate {
    QNetworkAccessManager *nm = nullptr;
    CppUtilities::DateTime lastCheck;
    UpdateCheckFlags flags = UpdateCheckFlags::Default;
    QNetworkRequest::CacheLoadControl cacheLoadControl = QNetworkRequest::PreferNetwork;
    QVersionNumber currentVersion = QVersionNumber();
    QString currentVersionSuffix = QString();
    QRegularExpression gitHubRegex = QRegularExpression(QStringLiteral(".*/github.com/([^/]+)/([^/]+)(/.*)?"));
    QRegularExpression gitHubRegex2 = QRegularExpression(QStringLiteral(".*/([^/.]+)\\.github.io/([^/]+)(/.*)?"));
    QRegularExpression assetRegex = QRegularExpression();
    QString executableName;
    QString previouslyFoundNewVersion;
    QString newVersion;
    QString latestVersion;
    QString additionalInfo;
    QString releaseNotes;
    QString error;
    QUrl downloadUrl;
    QUrl signatureUrl;
    QUrl releasesUrl;
    bool inProgress = false;
    bool updateAvailable = false;
    bool verbose = false;
};
#else
struct UpdateNotifierPrivate {
    QString error;
};
#endif

UpdateNotifier::UpdateNotifier(QObject *parent)
    : QObject(parent)
    , m_p(std::make_unique<UpdateNotifierPrivate>())
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    return;
#else
    m_p->verbose = qEnvironmentVariableIntValue(PROJECT_VARNAME_UPPER "_UPDATER_VERBOSE");

    const auto &appInfo = CppUtilities::applicationInfo;
    const auto url = QString::fromUtf8(appInfo.url);
    auto gitHubMatch = m_p->gitHubRegex.match(url);
    if (!gitHubMatch.hasMatch()) {
        gitHubMatch = m_p->gitHubRegex2.match(url);
    }
    const auto gitHubOrga = gitHubMatch.captured(1);
    const auto gitHubRepo = gitHubMatch.captured(2);
    if (gitHubOrga.isNull() || gitHubRepo.isNull()) {
        return;
    }
    const auto currentVersion = QString::fromUtf8(appInfo.version);
    auto suffixIndex = VersionSuffixIndex(-1);
    m_p->executableName = gitHubRepo + QT_UTILITIES_VERSION_SUFFIX;
    m_p->releasesUrl
        = QStringLiteral("https://api.github.com/repos/") % gitHubOrga % QChar('/') % gitHubRepo % QStringLiteral("/releases?per_page=25");
    m_p->currentVersion = QVersionNumber::fromString(currentVersion, &suffixIndex);
    m_p->currentVersionSuffix = suffixIndex >= 0 ? currentVersion.mid(suffixIndex) : QString();
#ifdef QT_UTILITIES_DOWNLOAD_REGEX
    m_p->assetRegex = QRegularExpression(m_p->executableName + QStringLiteral(QT_UTILITIES_DOWNLOAD_REGEX "\\..+"));
#endif
    if (m_p->verbose) {
        qDebug() << "deduced executable name: " << m_p->executableName;
        qDebug() << "assumed current version: " << m_p->currentVersion;
        qDebug() << "asset regex for current platform: " << m_p->assetRegex;
    }

    connect(this, &UpdateNotifier::checkedForUpdate, this, &UpdateNotifier::lastCheckNow);
#endif

#ifdef QT_UTILITIES_FAKE_NEW_VERSION_AVAILABLE
    QTimer::singleShot(10000, Qt::VeryCoarseTimer, this, [this] { emit updateAvailable(QStringLiteral("foo"), QString()); });
#endif
}

UpdateNotifier::~UpdateNotifier()
{
}

bool UpdateNotifier::isSupported() const
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    return false;
#else
    return !m_p->assetRegex.pattern().isEmpty();
#endif
}

bool UpdateNotifier::isInProgress() const
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    return false;
#else
    return m_p->inProgress;
#endif
}

bool UpdateNotifier::isUpdateAvailable() const
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    return false;
#else
    return m_p->updateAvailable;
#endif
}

UpdateCheckFlags UpdateNotifier::flags() const
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    return UpdateCheckFlags::None;
#else
    return m_p->flags;
#endif
}

void UpdateNotifier::setFlags(UpdateCheckFlags flags)
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    Q_UNUSED(flags)
#else
    m_p->flags = flags;
#endif
}

const QString &UpdateNotifier::executableName() const
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    static const auto v = QString();
    return v;
#else
    return m_p->executableName;
#endif
}

const QString &UpdateNotifier::newVersion() const
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    static const auto v = QString();
    return v;
#else
    return m_p->newVersion;
#endif
}

const QString &UpdateNotifier::latestVersion() const
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    static const auto v = QString();
    return v;
#else
    return m_p->latestVersion;
#endif
}

const QString &UpdateNotifier::additionalInfo() const
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    static const auto v = QString();
    return v;
#else
    return m_p->additionalInfo;
#endif
}

const QString &UpdateNotifier::releaseNotes() const
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    static const auto v = QString();
    return v;
#else
    return m_p->releaseNotes;
#endif
}

const QString &UpdateNotifier::error() const
{
    return m_p->error;
}

const QUrl &UpdateNotifier::downloadUrl() const
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    static const auto v = QUrl();
    return v;
#else
    return m_p->downloadUrl;
#endif
}

const QUrl &QtUtilities::UpdateNotifier::signatureUrl() const
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    static const auto v = QUrl();
    return v;
#else
    return m_p->signatureUrl;
#endif
}

CppUtilities::DateTime UpdateNotifier::lastCheck() const
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    return CppUtilities::DateTime();
#else
    return m_p->lastCheck;
#endif
}

void UpdateNotifier::restore(QSettings *settings)
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    Q_UNUSED(settings)
#else
    settings->beginGroup(QStringLiteral("updating"));
    m_p->newVersion = settings->value("newVersion").toString();
    m_p->latestVersion = settings->value("latestVersion").toString();
    m_p->releaseNotes = settings->value("releaseNotes").toString();
    m_p->downloadUrl = settings->value("downloadUrl").toUrl();
    m_p->signatureUrl = settings->value("signatureUrl").toUrl();
    m_p->lastCheck = CppUtilities::DateTime(settings->value("lastCheck").toULongLong());
    m_p->flags = static_cast<UpdateCheckFlags>(settings->value("flags").toULongLong());
    settings->endGroup();
#endif
}

void UpdateNotifier::save(QSettings *settings)
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    Q_UNUSED(settings)
#else
    settings->beginGroup(QStringLiteral("updating"));
    settings->setValue("newVersion", m_p->newVersion);
    settings->setValue("latestVersion", m_p->latestVersion);
    settings->setValue("releaseNotes", m_p->releaseNotes);
    settings->setValue("downloadUrl", m_p->downloadUrl);
    settings->setValue("signatureUrl", m_p->signatureUrl);
    settings->setValue("lastCheck", static_cast<qulonglong>(m_p->lastCheck.ticks()));
    settings->setValue("flags", static_cast<qulonglong>(m_p->flags));
    settings->endGroup();
#endif
}

QString UpdateNotifier::status() const
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    if (m_p->inProgress) {
        return tr("checking …");
    }
#endif
    if (!m_p->error.isEmpty()) {
        return tr("unable to check: %1").arg(m_p->error);
    }
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    if (!m_p->newVersion.isEmpty()) {
        return tr("new version available: %1 (last checked: %2)").arg(m_p->newVersion, QString::fromStdString(m_p->lastCheck.toIsoString()));
    } else if (!m_p->latestVersion.isEmpty()) {
        return tr("no new version available, latest release is: %1 (last checked: %2)")
            .arg(m_p->latestVersion, QString::fromStdString(m_p->lastCheck.toIsoString()));
    }
#endif
    return tr("unknown");
}

void UpdateNotifier::setNetworkAccessManager(QNetworkAccessManager *nm)
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    Q_UNUSED(nm)
#else
    m_p->nm = nm;
#endif
}

#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
void UpdateNotifier::setCacheLoadControl(QNetworkRequest::CacheLoadControl cacheLoadControl)
{
    m_p->cacheLoadControl = cacheLoadControl;
}
#endif

void UpdateNotifier::setError(const QString &context, QNetworkReply *reply)
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    Q_UNUSED(context)
    Q_UNUSED(reply)
#else
    m_p->error = context + reply->errorString();
    emit checkedForUpdate();
    emit inProgressChanged(m_p->inProgress = false);
#endif
}

void UpdateNotifier::setError(const QString &context, const QJsonParseError &jsonError, const QByteArray &response)
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    Q_UNUSED(context)
    Q_UNUSED(jsonError)
    Q_UNUSED(response)
#else
    m_p->error = context % jsonError.errorString() % QChar(' ') % QChar('(') % tr("at offset %1").arg(jsonError.offset) % QChar(')');
    if (!response.isEmpty()) {
        m_p->error += QStringLiteral("\nResponse was: ");
        m_p->error += QString::fromUtf8(response);
    }
    emit inProgressChanged(m_p->inProgress = false);
#endif
}

void UpdateNotifier::checkForUpdate()
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    m_p->error = tr("This build of the application does not support checking for updates.");
    emit inProgressChanged(false);
    return;
#else
    if (!m_p->nm || m_p->inProgress) {
        return;
    }
    emit inProgressChanged(m_p->inProgress = true);
    auto request = QNetworkRequest(m_p->releasesUrl);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, m_p->cacheLoadControl);
    auto *const reply = m_p->nm->get(request);
    connect(reply, &QNetworkReply::finished, this, &UpdateNotifier::readReleases);
#endif
}

void UpdateNotifier::resetUpdateInfo()
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    m_p->updateAvailable = false;
    m_p->downloadUrl.clear();
    m_p->signatureUrl.clear();
    m_p->latestVersion.clear();
    m_p->newVersion.clear();
    m_p->releaseNotes.clear();
#endif
}

void UpdateNotifier::lastCheckNow() const
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    m_p->lastCheck = CppUtilities::DateTime::now();
#endif
}

#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
/// \cond
static bool isVersionHigher(const QVersionNumber &lhs, const QString &lhsSuffix, const QVersionNumber &rhs, const QString &rhsSuffix)
{
    const auto cmp = QVersionNumber::compare(lhs, rhs);
    if (cmp > 0) {
        return true; // lhs is newer
    } else if (cmp < 0) {
        return false; // rhs is newer
    }
    if (!lhsSuffix.isEmpty() && rhsSuffix.isEmpty()) {
        return false; // lhs is pre-release and rhs is regular release, so rhs is newer
    }
    if (lhsSuffix.isEmpty() && !rhsSuffix.isEmpty()) {
        return true; // lhs is regular release and rhs is pre-release, so lhs is newer
    }
    // compare pre-release suffix
    return lhsSuffix > rhsSuffix;
}
/// \endcond
#endif

void UpdateNotifier::supplyNewReleaseData(const QByteArray &data)
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    Q_UNUSED(data)
#else
    // parse JSON
    auto jsonError = QJsonParseError();
    const auto replyDoc = QJsonDocument::fromJson(data, &jsonError);
    if (jsonError.error != QJsonParseError::NoError) {
        setError(tr("Unable to parse releases: "), jsonError, data);
        return;
    }
    resetUpdateInfo();
#if !defined(QT_JSON_READONLY)
    if (m_p->verbose) {
        qDebug().noquote() << "Update check: found releases: " << QString::fromUtf8(replyDoc.toJson(QJsonDocument::Indented));
    }
#endif
    // determine the release with the highest version (within the current page)
    const auto replyArray = replyDoc.array();
    const auto skipPreReleases = !(m_p->flags && UpdateCheckFlags::IncludePreReleases);
    const auto skipDrafts = !(m_p->flags && UpdateCheckFlags::IncludeDrafts);
    auto latestVersionFound = QVersionNumber();
    auto latestVersionSuffix = QString();
    auto latestVersionAssets = QJsonValue();
    auto latestVersionAssetsUrl = QString();
    auto latestVersionReleaseNotes = QString();
    for (const auto &releaseInfoVal : replyArray) {
        const auto releaseInfo = releaseInfoVal.toObject();
        const auto tag = releaseInfo.value(QLatin1String("tag_name")).toString();
        if ((skipPreReleases && releaseInfo.value(QLatin1String("prerelease")).toBool())
            || (skipDrafts && releaseInfo.value(QLatin1String("draft")).toBool())) {
            qDebug() << "Update check: skipping prerelease/draft: " << tag;
            continue;
        }
        auto suffixIndex = VersionSuffixIndex(-1);
        const auto versionStr = tag.startsWith(QChar('v')) ? tag.mid(1) : tag;
        const auto version = QVersionNumber::fromString(versionStr, &suffixIndex);
        const auto suffix = suffixIndex >= 0 ? versionStr.mid(suffixIndex) : QString();
        if (latestVersionFound.isNull() || isVersionHigher(version, suffix, latestVersionFound, latestVersionSuffix)) {
            latestVersionFound = version;
            latestVersionSuffix = suffix;
            latestVersionAssets = releaseInfo.value(QLatin1String("assets"));
            latestVersionAssetsUrl = releaseInfo.value(QLatin1String("assets_url")).toString();
            latestVersionReleaseNotes = releaseInfo.value(QLatin1String("body")).toString();
        }
        if (m_p->verbose) {
            qDebug() << "Update check: skipping release: " << tag;
        }
    }
    if (!latestVersionFound.isNull()) {
        m_p->latestVersion = latestVersionFound.toString() + latestVersionSuffix;
        m_p->releaseNotes = latestVersionReleaseNotes;
    }
    // process assets for latest version
    const auto foundUpdate
        = !latestVersionFound.isNull() && isVersionHigher(latestVersionFound, latestVersionSuffix, m_p->currentVersion, m_p->currentVersionSuffix);
    if (foundUpdate) {
        m_p->newVersion = latestVersionFound.toString() + latestVersionSuffix;
    }
    if (latestVersionAssets.isArray()) {
        return processAssets(latestVersionAssets.toArray(), foundUpdate);
    } else if (foundUpdate) {
        return queryRelease(latestVersionAssetsUrl);
    }
    emit checkedForUpdate();
    emit inProgressChanged(m_p->inProgress = false);
#endif
}

void UpdateNotifier::readReleases()
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    auto *const reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    switch (reply->error()) {
    case QNetworkReply::NoError: {
        supplyNewReleaseData(reply->readAll());
        break;
    }
    case QNetworkReply::OperationCanceledError:
        emit inProgressChanged(m_p->inProgress = false);
        return;
    default:
        setError(tr("Unable to request releases: "), reply);
    }
#endif
}

void UpdateNotifier::queryRelease(const QUrl &releaseUrl)
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    Q_UNUSED(releaseUrl)
#else
    auto request = QNetworkRequest(releaseUrl);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, m_p->cacheLoadControl);
    auto *const reply = m_p->nm->get(request);
    connect(reply, &QNetworkReply::finished, this, &UpdateNotifier::readRelease);
#endif
}

void UpdateNotifier::readRelease()
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    auto *const reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    switch (reply->error()) {
    case QNetworkReply::NoError: {
        // parse JSON
        auto jsonError = QJsonParseError();
        const auto response = reply->readAll();
        const auto replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            setError(tr("Unable to parse release: "), jsonError, response);
            return;
        }
#if !defined(QT_JSON_READONLY)
        if (m_p->verbose) {
            qDebug().noquote() << "Update check: found release info: " << QString::fromUtf8(replyDoc.toJson(QJsonDocument::Indented));
        }
#endif
        processAssets(replyDoc.object().value(QLatin1String("assets")).toArray(), true);
        break;
    }
    case QNetworkReply::OperationCanceledError:
        emit inProgressChanged(m_p->inProgress = false);
        return;
    default:
        setError(tr("Unable to request release: "), reply);
    }
#endif
}

void UpdateNotifier::processAssets(const QJsonArray &assets, bool forUpdate)
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    Q_UNUSED(assets)
    Q_UNUSED(forUpdate)
#else
    for (const auto &assetVal : assets) {
        if (!m_p->downloadUrl.isEmpty() && !m_p->signatureUrl.isEmpty()) {
            break;
        }
        const auto asset = assetVal.toObject();
        const auto assetName = asset.value(QLatin1String("name")).toString();
        if (assetName.isEmpty()) {
            continue;
        }
        if (m_p->assetRegex.match(assetName).hasMatch()) {
            const auto url = asset.value(QLatin1String("browser_download_url")).toString();
            if (assetName.endsWith(QLatin1String(".sig"))) {
                m_p->signatureUrl = url;
            } else {
                m_p->downloadUrl = url;
            }
            continue;
        }
        if (m_p->verbose) {
            qDebug() << "Update check: skipping asset: " << assetName;
        }
    }
    if (forUpdate) {
        m_p->updateAvailable = !m_p->downloadUrl.isEmpty();
    }
    emit checkedForUpdate();
    emit inProgressChanged(m_p->inProgress = false);
    if (forUpdate && m_p->updateAvailable && m_p->newVersion != m_p->previouslyFoundNewVersion) {
        // emit updateAvailable() only if we not have already previously emitted it for this version
        m_p->previouslyFoundNewVersion = m_p->newVersion;
        emit updateAvailable(m_p->newVersion, m_p->additionalInfo);
    }
#endif
}

#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
struct UpdaterPrivate {
    QNetworkAccessManager *nm = nullptr;
    QFile *fakeDownload = nullptr;
    QNetworkReply *currentDownload = nullptr;
    QNetworkReply *signatureDownload = nullptr;
    QNetworkRequest::CacheLoadControl cacheLoadControl = QNetworkRequest::PreferNetwork;
    QString error, statusMessage;
    QByteArray signature;
    QFutureWatcher<QPair<QString, QString>> watcher;
    QString executableName;
    QString signatureExtension;
    QRegularExpression executableRegex = QRegularExpression();
    QString storedPath;
    Updater::VerifyFunction verifyFunction;
};
#else
struct UpdaterPrivate {
    QString error;
};
#endif

Updater::Updater(const QString &executableName, QObject *parent)
    : Updater(executableName, QString(), parent)
{
}

Updater::Updater(const QString &executableName, const QString &signatureExtension, QObject *parent)
    : QObject(parent)
    , m_p(std::make_unique<UpdaterPrivate>())
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    Q_UNUSED(executableName)
    Q_UNUSED(signatureExtension)
#else
    connect(&m_p->watcher, &QFutureWatcher<void>::finished, this, &Updater::concludeUpdate);
    m_p->executableName = executableName;
    m_p->signatureExtension = signatureExtension;
    const auto signatureRegex = signatureExtension.isEmpty()
        ? QString()
        : QString(QStringLiteral("(") % QRegularExpression::escape(signatureExtension) % QStringLiteral(")?"));
#ifdef QT_UTILITIES_EXE_REGEX
    m_p->executableRegex = QRegularExpression(executableName % QStringLiteral(QT_UTILITIES_EXE_REGEX) % signatureRegex);
#endif
#endif
}

Updater::~Updater()
{
}

bool QtUtilities::Updater::isInProgress() const
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    return m_p->currentDownload != nullptr || m_p->signatureDownload != nullptr || m_p->watcher.isRunning();
#else
    return false;
#endif
}

QString QtUtilities::Updater::overallStatus() const
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    return isInProgress() ? tr("Update in progress …") : (m_p->error.isEmpty() ? tr("Update done") : tr("Update failed"));
#else
    return QString();
#endif
}

const QString &Updater::error() const
{
    return m_p->error;
}

const QString &QtUtilities::Updater::statusMessage() const
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    return m_p->statusMessage.isEmpty() ? m_p->error : m_p->statusMessage;
#else
    return m_p->error;
#endif
}

const QString &QtUtilities::Updater::storedPath() const
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    return m_p->storedPath;
#else
    static const auto empty = QString();
    return empty;
#endif
}

void Updater::setNetworkAccessManager(QNetworkAccessManager *nm)
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    m_p->nm = nm;
#else
    Q_UNUSED(nm)
#endif
}

void Updater::setVerifier(VerifyFunction &&verifyFunction)
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    m_p->verifyFunction = std::move(verifyFunction);
#else
    Q_UNUSED(verifyFunction)
#endif
}

#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
void Updater::setCacheLoadControl(QNetworkRequest::CacheLoadControl cacheLoadControl)
{
    m_p->cacheLoadControl = cacheLoadControl;
}
#endif

bool Updater::performUpdate(const QString &downloadUrl, const QString &signatureUrl)
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    Q_UNUSED(downloadUrl)
    Q_UNUSED(signatureUrl)
    setError(tr("This build of the application does not support self-updating."));
    return false;
#else
    if (isInProgress()) {
        return false;
    }
    startDownload(downloadUrl, signatureUrl);
    return true;
#endif
}

void Updater::abortUpdate()
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    if (m_p->currentDownload) {
        m_p->currentDownload->abort();
    }
    if (m_p->signatureDownload) {
        m_p->signatureDownload->abort();
    }
    if (m_p->watcher.isRunning()) {
        m_p->watcher.cancel();
    }
#endif
}

void Updater::setError(const QString &error)
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    m_p->statusMessage.clear();
#endif
    emit updateFailed(m_p->error = error);
    emit updateStatusChanged(m_p->error);
    emit updatePercentageChanged(0, 0);
    emit inProgressChanged(false);
}

void Updater::startDownload(const QString &downloadUrl, const QString &signatureUrl)
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    Q_UNUSED(downloadUrl)
    Q_UNUSED(signatureUrl)
#else
    m_p->error.clear();
    m_p->storedPath.clear();
    m_p->signature.clear();

    if (const auto fakeDownloadPath = qEnvironmentVariable(PROJECT_VARNAME_UPPER "_UPDATER_FAKE_DOWNLOAD"); !fakeDownloadPath.isEmpty()) {
        m_p->fakeDownload = new QFile(fakeDownloadPath);
        m_p->fakeDownload->open(QFile::ReadOnly);
        emit inProgressChanged(true);
        storeExecutable();
        return;
    }

    auto request = QNetworkRequest(QUrl(downloadUrl));
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, m_p->cacheLoadControl);
    m_p->statusMessage = tr("Downloading %1").arg(downloadUrl);
    m_p->currentDownload = m_p->nm->get(request);
    emit updateStatusChanged(m_p->statusMessage);
    emit updatePercentageChanged(0, 0);
    emit inProgressChanged(true);
    connect(m_p->currentDownload, &QNetworkReply::finished, this, &Updater::handleDownloadFinished);
    connect(m_p->currentDownload, &QNetworkReply::downloadProgress, this, &Updater::updatePercentageChanged);
    if (!signatureUrl.isEmpty()) {
        request.setUrl(signatureUrl);
        m_p->signatureDownload = m_p->nm->get(request);
        connect(m_p->signatureDownload, &QNetworkReply::finished, this, &Updater::handleDownloadFinished);
    }
#endif
}

void Updater::handleDownloadFinished()
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    if (m_p->signatureDownload && !m_p->signatureDownload->isFinished()) {
        emit updateStatusChanged(tr("Waiting for signature download …"));
        emit updatePercentageChanged(0, 0);
        return;
    }
    if (!m_p->currentDownload->isFinished()) {
        return;
    }

    if (m_p->signatureDownload) {
        readSignature();
        m_p->signatureDownload->deleteLater();
        m_p->signatureDownload = nullptr;
    }

    if (m_p->error.isEmpty()) {
        storeExecutable();
    } else {
        m_p->currentDownload->deleteLater();
    }
    m_p->currentDownload = nullptr;
#endif
}

void Updater::readSignature()
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    switch (m_p->signatureDownload->error()) {
    case QNetworkReply::NoError:
        m_p->signature = m_p->signatureDownload->readAll();
        break;
    default:
        setError(tr("Unable to download signature: ") + m_p->signatureDownload->errorString());
    }
#endif
}

void Updater::storeExecutable()
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    m_p->statusMessage = tr("Extracting …");
    emit updateStatusChanged(m_p->statusMessage);
    emit updatePercentageChanged(0, 0);
    auto *reply = static_cast<QIODevice *>(m_p->fakeDownload);
    auto archiveName = QString();
    auto hasError = false;
    if (reply) {
        archiveName = m_p->fakeDownload->fileName();
        hasError = m_p->fakeDownload->error() != QFileDevice::NoError;
    } else {
        reply = m_p->currentDownload;
        archiveName = m_p->currentDownload->request().url().fileName();
        hasError = m_p->currentDownload->error() != QNetworkReply::NoError;
    }
    if (hasError) {
        reply->deleteLater();
        setError(tr("Unable to download update: ") + reply->errorString());
        return;
    }
    auto res = QtConcurrent::run([this, reply, archiveName] {
        const auto data = reply->readAll();
        const auto dataView = std::string_view(data.data(), static_cast<std::size_t>(data.size()));
        auto foundExecutable = false, foundSignature = false;
        auto error = QString(), storePath = QString();
        auto newExeName = std::string(), signatureName = std::string();
        auto newExeData = std::string();
        auto newExe = QFile();
        reply->deleteLater();

        // determine current executable path
        const auto appDirPath = QCoreApplication::applicationDirPath();
        const auto appFilePath = QCoreApplication::applicationFilePath();
        if (appDirPath.isEmpty() || appFilePath.isEmpty()) {
            error = tr("Unable to determine application path.");
            return QPair<QString, QString>(error, storePath);
        }

        // handle cancellations
        const auto checkCancellation = [this, &error] {
            if (m_p->watcher.isCanceled()) {
                error = tr("Extraction was cancelled.");
                return true;
            } else {
                return false;
            }
        };
        if (checkCancellation()) {
            return QPair<QString, QString>(error, storePath);
        }

        try {
            CppUtilities::walkThroughArchiveFromBuffer(
                dataView, archiveName.toStdString(),
                [this](const char *filePath, const char *fileName, mode_t mode) {
                    Q_UNUSED(filePath)
                    Q_UNUSED(mode)
                    if (m_p->watcher.isCanceled()) {
                        return true;
                    }
                    return m_p->executableRegex.match(QString::fromUtf8(fileName)).hasMatch();
                },
                [&](std::string_view path, CppUtilities::ArchiveFile &&file) {
                    Q_UNUSED(path)
                    if (checkCancellation()) {
                        return true;
                    }
                    if (file.type != CppUtilities::ArchiveFileType::Regular) {
                        return false;
                    }

                    // read signature file
                    const auto fileName = QString::fromUtf8(file.name.data(), static_cast<QString::size_type>(file.name.size()));
                    if (!m_p->signatureExtension.isEmpty() && fileName.endsWith(m_p->signatureExtension)) {
                        m_p->signature = QByteArray::fromStdString(file.content);
                        foundSignature = true;
                        signatureName = file.name;
                        return foundExecutable && foundSignature;
                    }

                    // write executable from archive to disk (using a temporary filename)
                    foundExecutable = true;
                    newExeName = file.name;
                    newExe.setFileName(appDirPath % QChar('/') % fileName % QStringLiteral(".tmp"));
                    if (!newExe.open(QFile::WriteOnly | QFile::Truncate)) {
                        error = tr("Unable to create new executable under \"%1\": %2").arg(newExe.fileName(), newExe.errorString());
                        return true;
                    }
                    const auto size = static_cast<qint64>(file.content.size());
                    if (!(newExe.write(file.content.data(), size) == size) || !newExe.flush()) {
                        error = tr("Unable to write new executable under \"%1\": %2").arg(newExe.fileName(), newExe.errorString());
                        return true;
                    }
                    if (!newExe.setPermissions(
                            newExe.permissions() | QFileDevice::ExeOwner | QFileDevice::ExeUser | QFileDevice::ExeGroup | QFileDevice::ExeOther)) {
                        error = tr("Unable to make new binary under \"%1\" executable.").arg(newExe.fileName());
                        return true;
                    }

                    storePath = newExe.fileName();
                    newExeData = std::move(file.content);
                    return foundExecutable && foundSignature;
                });
        } catch (const CppUtilities::ArchiveException &e) {
            error = tr("Unable to open downloaded archive: %1").arg(e.what());
        }
        if (error.isEmpty() && foundExecutable) {
            // verify whether downloaded binary is valid if a verify function was assigned
            if (m_p->verifyFunction) {
                if (const auto verifyError = m_p->verifyFunction(Updater::Update{ .executableName = newExeName,
                        .signatureName = signatureName,
                        .data = newExeData,
                        .signature = std::string_view(m_p->signature.data(), static_cast<std::size_t>(m_p->signature.size())) });
                    !verifyError.isEmpty()) {
                    error = tr("Unable to verify whether downloaded binary is valid: %1").arg(verifyError);
                    return QPair<QString, QString>(error, storePath);
                }
            }

            // rename current executable to keep it as backup
            auto currentExeInfo = QFileInfo(appFilePath);
            auto currentExe = QFile(appFilePath);
            const auto completeSuffix = currentExeInfo.completeSuffix();
            const auto suffixWithDot = completeSuffix.isEmpty() ? QString() : QChar('.') + completeSuffix;
            for (auto i = 0; i < 100; ++i) {
                const auto backupNumber = i ? QString::number(i) : QString();
                const auto backupPath = QString(currentExeInfo.path() % QChar('/') % currentExeInfo.baseName() % QStringLiteral("-backup")
                    % backupNumber % QChar('-') % QString::fromUtf8(CppUtilities::applicationInfo.version) % suffixWithDot);
                if (QFile::exists(backupPath)) {
                    continue;
                }
                if (!currentExe.rename(backupPath)) {
                    error = tr("Unable to move current executable to \"%1\": %2").arg(backupPath, currentExe.errorString());
                    return QPair<QString, QString>(error, storePath);
                }
                break;
            }

            // rename new executable to use it in place of current executable
            if (!newExe.rename(appFilePath)) {
                error = tr("Unable to rename new executable \"%1\" to \"%2\": %3").arg(newExe.fileName(), appFilePath, newExe.errorString());
                return QPair<QString, QString>(error, storePath);
            }
            storePath = newExe.fileName();
        }
        if (error.isEmpty() && !foundExecutable) {
            error = tr("Unable to find executable in downloaded archive.");
        }
        return QPair<QString, QString>(error, storePath);
    });
    m_p->watcher.setFuture(std::move(res));
#endif
}

void Updater::concludeUpdate()
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    auto res = m_p->watcher.result();
    m_p->error = res.first;
    m_p->storedPath = res.second;
    if (!m_p->error.isEmpty()) {
        m_p->statusMessage.clear();
        emit updateFailed(m_p->error);
    } else {
        m_p->statusMessage = tr("Update stored under: %1").arg(m_p->storedPath);
        emit updateStored();
    }
    emit updateStatusChanged(statusMessage());
    emit updatePercentageChanged(0, 0);
    emit inProgressChanged(false);
#endif
}

struct UpdateHandlerPrivate {
    explicit UpdateHandlerPrivate(const QString &executableName, const QString &signatureExtension)
        : updater(executableName.isEmpty() ? notifier.executableName() : executableName, signatureExtension)
    {
    }

    UpdateNotifier notifier;
    Updater updater;
    QTimer timer;
    QSettings *settings;
    std::optional<UpdateHandler::CheckInterval> checkInterval;
    bool hasCheckedOnceSinceStartup = false;
    bool considerSeparateSignature = false;
};

UpdateHandler *UpdateHandler::s_mainInstance = nullptr;

/*!
 * \brief Handles checking for updates and performing an update of the application if available.
 */
UpdateHandler::UpdateHandler(QSettings *settings, QNetworkAccessManager *nm, QObject *parent)
    : QtUtilities::UpdateHandler(QString(), QString(), settings, nm, parent)
{
}

/*!
 * \brief Handles checking for updates and performing an update of the application if available.
 */
UpdateHandler::UpdateHandler(
    const QString &executableName, const QString &signatureExtension, QSettings *settings, QNetworkAccessManager *nm, QObject *parent)
    : QObject(parent)
    , m_p(std::make_unique<UpdateHandlerPrivate>(executableName, signatureExtension))
{
    m_p->notifier.setNetworkAccessManager(nm);
    m_p->updater.setNetworkAccessManager(nm);
    m_p->timer.setSingleShot(true);
    m_p->timer.setTimerType(Qt::VeryCoarseTimer);
    m_p->settings = settings;
    connect(&m_p->timer, &QTimer::timeout, &m_p->notifier, &UpdateNotifier::checkForUpdate);
    connect(&m_p->notifier, &UpdateNotifier::checkedForUpdate, this, &UpdateHandler::handleUpdateCheckDone);
}

UpdateHandler::~UpdateHandler()
{
}

UpdateNotifier *UpdateHandler::notifier()
{
    return &m_p->notifier;
}

Updater *UpdateHandler::updater()
{
    return &m_p->updater;
}

const UpdateHandler::CheckInterval &UpdateHandler::checkInterval() const
{
    if (m_p->checkInterval.has_value()) {
        return m_p->checkInterval.value();
    }
    m_p->settings->beginGroup(QStringLiteral("updating"));
    auto &checkInterval = m_p->checkInterval.emplace();
    checkInterval.duration = CppUtilities::TimeSpan::fromMilliseconds(m_p->settings->value("checkIntervalMs", 60 * 60 * 1000).toInt());
    checkInterval.enabled = m_p->settings->value("automaticChecksEnabled", false).toBool();
    m_p->settings->endGroup();
    return checkInterval;
}

void UpdateHandler::setCheckInterval(CheckInterval checkInterval)
{
    m_p->checkInterval = checkInterval;
    m_p->settings->beginGroup(QStringLiteral("updating"));
    m_p->settings->setValue("checkIntervalMs", checkInterval.duration.totalMilliseconds());
    m_p->settings->setValue("automaticChecksEnabled", checkInterval.enabled);
    m_p->settings->endGroup();
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    scheduleNextUpdateCheck();
#endif
}

bool UpdateHandler::isConsideringSeparateSignature() const
{
    return m_p->considerSeparateSignature;
}

void UpdateHandler::setConsideringSeparateSignature(bool consideringSeparateSignature)
{
    m_p->considerSeparateSignature = consideringSeparateSignature;
}

#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
void UpdateHandler::setCacheLoadControl(QNetworkRequest::CacheLoadControl cacheLoadControl)
{
    m_p->notifier.setCacheLoadControl(cacheLoadControl);
    m_p->updater.setCacheLoadControl(cacheLoadControl);
}
#endif

void UpdateHandler::applySettings()
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    m_p->notifier.restore(m_p->settings);
    scheduleNextUpdateCheck();
#endif
}

void UpdateHandler::performUpdate()
{
    m_p->updater.performUpdate(
        m_p->notifier.downloadUrl().toString(), m_p->considerSeparateSignature ? m_p->notifier.signatureUrl().toString() : QString());
}

void UpdateHandler::saveNotifierState()
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    m_p->notifier.save(m_p->settings);
#endif
}

void UpdateHandler::handleUpdateCheckDone()
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    saveNotifierState();
    scheduleNextUpdateCheck();
#endif
}

#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
void UpdateHandler::scheduleNextUpdateCheck()
{
    m_p->timer.stop();

    const auto &interval = checkInterval();
    if (!interval.enabled || (interval.duration.isNull() && m_p->hasCheckedOnceSinceStartup)) {
        return;
    }
    const auto timeLeft = interval.duration - (CppUtilities::DateTime::now() - m_p->notifier.lastCheck());
    std::cerr << CppUtilities::EscapeCodes::Phrases::Info
              << "Check for updates due in: " << timeLeft.toString(CppUtilities::TimeSpanOutputFormat::WithMeasures)
              << CppUtilities::EscapeCodes::Phrases::End;
    m_p->hasCheckedOnceSinceStartup = true; // the attempt counts
    m_p->timer.start(std::max(1000, static_cast<int>(timeLeft.totalMilliseconds())));
}
#endif

void RestartHandler::requestRestart()
{
    m_restartRequested = true;
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    QCoreApplication::quit();
#endif
}

void RestartHandler::respawnIfRestartRequested()
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    if (!m_restartRequested) {
        return;
    }
    auto *const process = new QProcess(QCoreApplication::instance());
    auto args = QCoreApplication::arguments();
    args.removeFirst();
    process->setProgram(QCoreApplication::applicationFilePath());
    process->setArguments(args);
    process->startDetached();
#endif
}

#ifdef QT_UTILITIES_GUI_QTWIDGETS
struct UpdateOptionPagePrivate {
    UpdateOptionPagePrivate(UpdateHandler *updateHandler)
        : updateHandler(updateHandler)
    {
    }
    UpdateHandler *updateHandler = nullptr;
    std::function<void()> restartHandler;
};

UpdateOptionPage::UpdateOptionPage(UpdateHandler *updateHandler, QWidget *parentWidget)
    : UpdateOptionPageBase(parentWidget)
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    , m_p(std::make_unique<UpdateOptionPagePrivate>(updateHandler))
#endif
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    Q_UNUSED(updateHandler)
#endif
}

UpdateOptionPage::~UpdateOptionPage()
{
}

void UpdateOptionPage::setRestartHandler(std::function<void()> &&handler)
{
    m_p->restartHandler = std::move(handler);
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    if (ui() && m_p->restartHandler) {
        QObject::connect(ui()->restartPushButton, &QPushButton::clicked, widget(), m_p->restartHandler);
    }
#endif
}

bool UpdateOptionPage::apply()
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    if (!m_p->updateHandler) {
        return true;
    }
    m_p->updateHandler->setCheckInterval(UpdateHandler::CheckInterval{
        .duration = CppUtilities::TimeSpan::fromMinutes(ui()->checkIntervalSpinBox->value()), .enabled = ui()->enabledCheckBox->isChecked() });
    auto flags = UpdateCheckFlags::None;
    CppUtilities::modFlagEnum(flags, UpdateCheckFlags::IncludePreReleases, ui()->preReleasesCheckBox->isChecked());
    CppUtilities::modFlagEnum(flags, UpdateCheckFlags::IncludeDrafts, ui()->draftsCheckBox->isChecked());
    m_p->updateHandler->notifier()->setFlags(flags);
    m_p->updateHandler->saveNotifierState();
#endif
    return true;
}

void UpdateOptionPage::reset()
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    if (!m_p->updateHandler) {
        return;
    }
    const auto &checkInterval = m_p->updateHandler->checkInterval();
    ui()->checkIntervalSpinBox->setValue(static_cast<int>(checkInterval.duration.totalMinutes()));
    ui()->enabledCheckBox->setChecked(checkInterval.enabled);
    const auto flags = m_p->updateHandler->notifier()->flags();
    ui()->preReleasesCheckBox->setChecked(flags && UpdateCheckFlags::IncludePreReleases);
    ui()->draftsCheckBox->setChecked(flags && UpdateCheckFlags::IncludeDrafts);
#endif
}

QWidget *UpdateOptionPage::setupWidget()
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    if (m_p->updateHandler && m_p->updateHandler->notifier()->isSupported()) {
        auto *const widget = UpdateOptionPageBase::setupWidget(); // call base implementation first, so ui() is available
        ui()->versionInUseValueLabel->setText(QString::fromUtf8(CppUtilities::applicationInfo.version));
        ui()->updateWidget->hide();
        ui()->releaseNotesPushButton->hide();
        updateLatestVersion();
        QObject::connect(ui()->checkNowPushButton, &QPushButton::clicked, m_p->updateHandler->notifier(), &UpdateNotifier::checkForUpdate);
        QObject::connect(ui()->updatePushButton, &QPushButton::clicked, m_p->updateHandler, &UpdateHandler::performUpdate);
        QObject::connect(ui()->abortUpdatePushButton, &QPushButton::clicked, m_p->updateHandler->updater(), &Updater::abortUpdate);
        if (m_p->restartHandler) {
            QObject::connect(ui()->restartPushButton, &QPushButton::clicked, widget, m_p->restartHandler);
        }
        QObject::connect(ui()->releaseNotesPushButton, &QPushButton::clicked, widget, [this, widget] {
            const auto *const notifier = m_p->updateHandler->notifier();
            QMessageBox::information(widget, QCoreApplication::applicationName(),
                QCoreApplication::translate("QtGui::UpdateOptionPage", "<strong>Release notes of version %1:</strong><br>")
                        .arg(notifier->latestVersion())
                    + notifier->releaseNotes());
        });
        QObject::connect(
            m_p->updateHandler->notifier(), &UpdateNotifier::inProgressChanged, widget, [this](bool inProgress) { updateLatestVersion(inProgress); });
        QObject::connect(m_p->updateHandler->updater(), &Updater::inProgressChanged, widget, [this](bool inProgress) {
            const auto *const updater = m_p->updateHandler->updater();
            ui()->updateWidget->setVisible(true);
            ui()->updateInProgressLabel->setText(updater->overallStatus());
            ui()->updateProgressBar->setVisible(inProgress);
            ui()->abortUpdatePushButton->setVisible(inProgress);
            ui()->restartPushButton->setVisible(!inProgress && !updater->storedPath().isEmpty() && updater->error().isEmpty());
        });
        QObject::connect(m_p->updateHandler->updater(), &Updater::updateStatusChanged, widget,
            [this](const QString &statusMessage) { ui()->updateStatusLabel->setText(statusMessage); });
        QObject::connect(m_p->updateHandler->updater(), &Updater::updatePercentageChanged, widget, [this](qint64 bytesReceived, qint64 bytesTotal) {
            if (bytesTotal == 0) {
                ui()->updateProgressBar->setMaximum(0);
            } else {
                ui()->updateProgressBar->setValue(static_cast<int>(bytesReceived * 100 / bytesTotal));
                ui()->updateProgressBar->setMaximum(100);
            }
        });
        return widget;
    }
#endif

    auto *const label = new QLabel;
    label->setWindowTitle(QCoreApplication::translate("QtGui::UpdateOptionPage", "Updating"));
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    label->setText(QCoreApplication::translate("QtUtilities::UpdateOptionPage", "Checking for updates is not supported on this platform."));
#else
    label->setText(QCoreApplication::translate("QtUtilities::UpdateOptionPage",
        "This build of %1 has automatic updates disabled. You may update the application in an automated way via your package manager, though.")
            .arg(CppUtilities::applicationInfo.name));
#endif
    return label;
}

void UpdateOptionPage::updateLatestVersion(bool)
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    if (!m_p->updateHandler) {
        return;
    }
    const auto &notifier = *m_p->updateHandler->notifier();
    const auto &downloadUrl = notifier.downloadUrl();
    const auto downloadUrlEscaped = downloadUrl.toString().toHtmlEscaped();
    ui()->latestVersionValueLabel->setText(notifier.status());
    ui()->downloadUrlLabel->setText(downloadUrl.isEmpty()
            ? (notifier.latestVersion().isEmpty()
                      ? QCoreApplication::translate("QtUtilities::UpdateOptionPage", "no new version available for download")
                      : QCoreApplication::translate(
                            "QtUtilities::UpdateOptionPage", "new version available but no build for the current platform present yet"))
            : (QStringLiteral("<a href=\"") % downloadUrlEscaped % QStringLiteral("\">") % downloadUrlEscaped % QStringLiteral("</a>")));
    ui()->updatePushButton->setDisabled(downloadUrl.isEmpty());
    ui()->releaseNotesPushButton->setHidden(notifier.releaseNotes().isEmpty());
#endif
}

VerificationErrorMessageBox::VerificationErrorMessageBox()
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    setWindowTitle(QCoreApplication::applicationName());
    setStandardButtons(QMessageBox::Cancel | QMessageBox::Ignore);
    setDefaultButton(QMessageBox::Cancel);
    setIcon(QMessageBox::Critical);
#endif
}

VerificationErrorMessageBox::~VerificationErrorMessageBox()
{
}

int VerificationErrorMessageBox::execForError(QString &errorMessage, const QString &explanation)
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    auto loop = QEventLoop();
    QObject::connect(this, &QDialog::finished, &loop, &QEventLoop::exit);
    QMetaObject::invokeMethod(this, "openForError", Qt::QueuedConnection, Q_ARG(QString, errorMessage), Q_ARG(QString, explanation));
    auto res = loop.exec();
    if (res == QMessageBox::Ignore) {
        errorMessage.clear();
    }
    return res;
#else
    Q_UNUSED(errorMessage)
    Q_UNUSED(explanation)
    return 0;
#endif
}

void VerificationErrorMessageBox::openForError(const QString &errorMessage, const QString &explanation)
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    setText(tr("<p>The signature of the downloaded executable could not be verified: %1</p>").arg(errorMessage) + explanation);
    open();
#else
    Q_UNUSED(errorMessage)
    Q_UNUSED(explanation)
#endif
}

struct UpdateDialogPrivate {
    UpdateOptionPage *updateOptionPage = nullptr;
};

UpdateDialog::UpdateDialog(QWidget *parent)
    : SettingsDialog(parent)
    , m_p(std::make_unique<UpdateDialogPrivate>())
{
    auto *const category = new OptionCategory;
    m_p->updateOptionPage = new UpdateOptionPage(UpdateHandler::mainInstance(), this);
    category->assignPages({ m_p->updateOptionPage });
    setWindowTitle(m_p->updateOptionPage->widget()->windowTitle());
    setTabBarAlwaysVisible(false);
    setSingleCategory(category);
}

UpdateDialog::~UpdateDialog()
{
}

UpdateOptionPage *UpdateDialog::page()
{
    return m_p->updateOptionPage;
}

const UpdateOptionPage *UpdateDialog::page() const
{
    return m_p->updateOptionPage;
}

#endif

} // namespace QtUtilities

#if defined(QT_UTILITIES_GUI_QTWIDGETS)
INSTANTIATE_UI_FILE_BASED_OPTION_PAGE(UpdateOptionPage)
#endif
