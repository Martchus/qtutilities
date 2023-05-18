#include "./updater.h"

#include "resources/config.h"

#include <QSettings>
#include <QTimer>

#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
#include <c++utilities/application/argumentparser.h>
#include <c++utilities/io/ansiescapecodes.h>
#include <c++utilities/io/archive.h>

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QStringBuilder>
#include <QVersionNumber>
#include <QtConcurrentRun>
#include <QtProcessorDetection>
#include <QtSystemDetection>

#if defined(QT_UTILITIES_GUI_QTWIDGETS)
#include <QMessageBox>
#endif

#include <iostream>
#endif

#if defined(QT_UTILITIES_GUI_QTWIDGETS)
#include "ui_updateoptionpage.h"
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
#define QT_UTILITIES_VERSION_SUFFIX QString()
#else
#define QT_UTILITIES_VERSION_SUFFIX QStringLiteral("-qt5")
#endif

#if defined(Q_OS_WIN64)
#if defined(Q_PROCESSOR_X86_64)
#define QT_UTILITIES_EXE_REGEX "-.*-x86_64-w64-mingw32\\.exe"
#elif defined(Q_PROCESSOR_ARM_64)
#define QT_UTILITIES_EXE_REGEX "-.*-aarch64-w64-mingw32\\.exe"
#endif
#elif defined(Q_OS_WIN32)
#define QT_UTILITIES_EXE_REGEX "-.*-i686-w64-mingw32\\.exe"
#elif defined(__GNUC__) && defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#if defined(Q_PROCESSOR_X86_64)
#define QT_UTILITIES_EXE_REGEX "-.*-x86_64-pc-linux-gnu"
#elif defined(Q_PROCESSOR_ARM_64)
#define QT_UTILITIES_EXE_REGEX "-.*-aarch64-pc-linux-gnu"
#endif
#endif

namespace QtUtilities {

#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
struct UpdateNotifierPrivate {
    QNetworkAccessManager *nm = nullptr;
    CppUtilities::DateTime lastCheck;
    QNetworkRequest::CacheLoadControl cacheLoadControl = QNetworkRequest::PreferNetwork;
    QVersionNumber currentVersion = QVersionNumber();
    QRegularExpression gitHubRegex = QRegularExpression(QStringLiteral(".*/github.com/([^/]+)/([^/]+)(/.*)?"));
    QRegularExpression gitHubRegex2 = QRegularExpression(QStringLiteral(".*/([^/.]+)\\.github.io/([^/]+)(/.*)?"));
    QRegularExpression assetRegex = QRegularExpression();
    QString executableName;
    QString newVersion;
    QString latestVersion;
    QString additionalInfo;
    QString error;
    QUrl downloadUrl;
    QUrl releasesUrl;
    bool inProgress = false;
    bool updateAvailable = false;
    bool verbose = false;
};
#else
struct UpdateNotifierPrivate {};
#endif

UpdateNotifier::UpdateNotifier(QObject *parent)
    : QObject(parent)
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    , m_p(std::make_unique<UpdateNotifierPrivate>())
#endif
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
    m_p->executableName = gitHubRepo + QT_UTILITIES_VERSION_SUFFIX;
    m_p->releasesUrl
        = QStringLiteral("https://api.github.com/repos/") % gitHubOrga % QChar('/') % gitHubRepo % QStringLiteral("/releases?per_page=25");
    m_p->currentVersion = QVersionNumber::fromString(QUtf8StringView(appInfo.version));
#ifdef QT_UTILITIES_EXE_REGEX
    m_p->assetRegex = QRegularExpression(m_p->executableName + QStringLiteral(QT_UTILITIES_EXE_REGEX "\\..+"));
#endif

    connect(this, &UpdateNotifier::checkedForUpdate, this, &UpdateNotifier::lastCheckNow);
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

const QString &UpdateNotifier::error() const
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    static const auto v = QString();
    return v;
#else
    return m_p->error;
#endif
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
    settings->beginGroup(QStringLiteral("updating"));
    m_p->newVersion = settings->value("newVersion").toString();
    m_p->latestVersion = settings->value("latestVersion").toString();
    m_p->downloadUrl = settings->value("downloadUrl").toUrl();
    m_p->lastCheck = CppUtilities::DateTime(settings->value("lastCheck").toULongLong());
    settings->endGroup();
}

void UpdateNotifier::save(QSettings *settings)
{
    settings->beginGroup(QStringLiteral("updating"));
    settings->setValue("newVersion", m_p->newVersion);
    settings->setValue("latestVersion", m_p->latestVersion);
    settings->setValue("downloadUrl", m_p->downloadUrl);
    settings->setValue("lastCheck", static_cast<qulonglong>(m_p->lastCheck.ticks()));
    settings->endGroup();
}

QString UpdateNotifier::status() const
{
    if (m_p->inProgress) {
        return tr("checking …");
    } else if (!m_p->error.isEmpty()) {
        return tr("unable to check: %1").arg(m_p->error);
    } else if (!m_p->newVersion.isEmpty()) {
        return tr("new version available: %1 (last checked: %2)").arg(m_p->newVersion, QString::fromStdString(m_p->lastCheck.toIsoString()));
    } else if (!m_p->latestVersion.isEmpty()) {
        return tr("no new version available, latest release is: %1 (last checked: %2)")
            .arg(m_p->latestVersion, QString::fromStdString(m_p->lastCheck.toIsoString()));
    } else {
        return tr("unknown");
    }
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
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    m_p->error = context + reply->errorString();
    emit checkedForUpdate();
    emit inProgressChanged(m_p->inProgress = false);
#endif
}

void UpdateNotifier::setError(const QString &context, const QJsonParseError &jsonError, const QByteArray &response, QNetworkReply *)
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
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
    m_p->newVersion.clear();
#endif
}

void UpdateNotifier::lastCheckNow() const
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    m_p->lastCheck = CppUtilities::DateTime::now();
#endif
}

void UpdateNotifier::readReleases()
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    auto *const reply = static_cast<QNetworkReply *>(sender());
    switch (reply->error()) {
    case QNetworkReply::NoError: {
        // parse JSON
        auto jsonError = QJsonParseError();
        const auto response = reply->readAll();
        const auto replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            setError(tr("Unable to parse releases: "), jsonError, response, reply);
            return;
        }
#if !defined(QT_JSON_READONLY)
        if (m_p->verbose) {
            qDebug().noquote() << "Update check: found releases: " << QString::fromUtf8(replyDoc.toJson(QJsonDocument::Indented));
        }
#endif
        const auto replyArray = replyDoc.array();
        auto latestVersionFound = QVersionNumber();
        auto latestVersionAsset = QJsonValue();
        for (const auto &releaseInfoVal : replyArray) {
            const auto releaseInfo = releaseInfoVal.toObject();
            const auto tag = releaseInfo.value(QLatin1String("tag_name")).toString();
            const auto version = QVersionNumber::fromString(tag.startsWith(QChar('v')) ? tag.mid(1) : tag);
            const auto assets = releaseInfo.value(QLatin1String("assets"));
            if (latestVersionFound.isNull() || version > latestVersionFound) {
                latestVersionFound = version;
                latestVersionAsset = assets;
            }
            if (!version.isNull() && version > m_p->currentVersion) {
                m_p->latestVersion = latestVersionFound.toString();
                m_p->newVersion = version.toString();
                if (assets.isArray()) {
                    processAssets(assets.toArray(), true);
                    emit inProgressChanged(m_p->inProgress = false);
                } else {
                    queryRelease(releaseInfo.value(QLatin1String("assets_url")).toString());
                }
                return;
            }
            if (m_p->verbose) {
                qDebug() << "Update check: skipping release: " << tag;
            }
        }
        m_p->latestVersion = latestVersionFound.toString();
        if (latestVersionAsset.isArray()) {
            processAssets(latestVersionAsset.toArray(), false);
        }
        emit checkedForUpdate();
        emit inProgressChanged(m_p->inProgress = false);
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
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
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
    switch (reply->error()) {
    case QNetworkReply::NoError: {
        // parse JSON
        auto jsonError = QJsonParseError();
        const auto response = reply->readAll();
        const auto replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            setError(tr("Unable to parse release: "), jsonError, response, reply);
            return;
        }
#if !defined(QT_JSON_READONLY)
        if (m_p->verbose) {
            qDebug().noquote() << "Update check: found release info: " << QString::fromUtf8(replyDoc.toJson(QJsonDocument::Indented));
        }
#endif
        processAssets(replyDoc.object().value(QLatin1String("assets")).toArray(), true);
        emit inProgressChanged(m_p->inProgress = false);
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
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    for (const auto &assetVal : assets) {
        const auto asset = assetVal.toObject();
        const auto assetName = asset.value(QLatin1String("name")).toString();
        if (!assetName.isEmpty() && m_p->assetRegex.match(assetName).hasMatch()) {
            m_p->downloadUrl = asset.value(QLatin1String("browser_download_url")).toString();
            if (forUpdate) {
                m_p->updateAvailable = true;
                emit checkedForUpdate();
                emit updateAvailable(m_p->newVersion, m_p->additionalInfo);
            }
            return;
        }
        if (m_p->verbose) {
            qDebug() << "Update check: skipping asset: " << assetName;
        }
    }
    emit checkedForUpdate();
#endif
}

#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
struct UpdaterPrivate {
    QNetworkAccessManager *nm = nullptr;
    QNetworkRequest::CacheLoadControl cacheLoadControl = QNetworkRequest::PreferNetwork;
    QString error;
    QFutureWatcher<QString> watcher;
    QString executableName;
    QRegularExpression executableRegex = QRegularExpression();
};
#else
struct UpdaterPrivate {};
#endif

Updater::Updater(const QString &executableName, QObject *parent)
    : QObject(parent)
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    , m_p(std::make_unique<UpdaterPrivate>())
#endif
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    connect(&m_p->watcher, &QFutureWatcher<void>::finished, this, &Updater::handleUpdateStored);
    m_p->executableName = executableName;
#ifdef QT_UTILITIES_EXE_REGEX
    m_p->executableRegex = QRegularExpression(executableName + QStringLiteral(QT_UTILITIES_EXE_REGEX));
#endif
#endif
}

Updater::~Updater()
{
}

const QString &Updater::error() const
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    return m_p->error;
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

#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
void Updater::setCacheLoadControl(QNetworkRequest::CacheLoadControl cacheLoadControl)
{
    m_p->cacheLoadControl = cacheLoadControl;
}
#endif

void Updater::performUpdate(const QString &downloadUrl)
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    Q_UNUSED(downloadUrl)
    setError(tr("This build of the application does not support self-updating."));
#else
    startDownload(downloadUrl);
#endif
}

void Updater::setError(const QString &error)
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    emit updateFailed(m_p->error = error);
#endif
}

void Updater::startDownload(const QString &downloadUrl)
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    auto request = QNetworkRequest(QUrl(downloadUrl));
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, m_p->cacheLoadControl);
    auto *const reply = m_p->nm->get(request);
    connect(reply, &QNetworkReply::finished, this, &Updater::storeExecutable);
#endif
}

void Updater::storeExecutable()
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    switch (auto *const reply = static_cast<QNetworkReply *>(sender()); reply->error()) {
    case QNetworkReply::NoError: {
        auto res = QtConcurrent::run([this, reply] {
            const auto data = reply->readAll();
            const auto dataView = std::string_view(data.data(), static_cast<std::size_t>(data.size()));
            const auto archiveName = reply->request().url().fileName().toStdString();
            auto foundExecutable = false;
            auto error = QString();
            try {
                CppUtilities::walkThroughArchiveFromBuffer(
                    dataView, archiveName,
                    [this](const char *filePath, const char *fileName, mode_t mode) {
                        Q_UNUSED(filePath)
                        Q_UNUSED(mode)
                        return m_p->executableRegex.match(QString::fromUtf8(fileName)).hasMatch();
                    },
                    [&foundExecutable, &error](std::string_view path, CppUtilities::ArchiveFile &&file) {
                        Q_UNUSED(path)
                        if (file.type != CppUtilities::ArchiveFileType::Regular) {
                            return false;
                        }
                        foundExecutable = true;
                        const auto appDirPath = QCoreApplication::applicationDirPath();
                        const auto appFilePath = QCoreApplication::applicationFilePath();
                        if (appDirPath.isEmpty() || appFilePath.isEmpty()) {
                            error = tr("Unable to determine application path.");
                            return true;
                        }
                        auto newExe = QFile(appDirPath % QChar('/') % QString::fromUtf8(file.name));
                        if (!newExe.open(QFile::WriteOnly)) {
                            error = tr("Unable to create new executable under \"%1\": %2").arg(newExe.fileName(), newExe.errorString());
                            return true;
                        }
                        const auto size = static_cast<qint64>(file.content.size());
                        if (!(newExe.write(file.content.data(), size) == size) || !newExe.flush()) {
                            error = tr("Unable to write new executable under \"%1\": %2").arg(newExe.fileName(), newExe.errorString());
                            return true;
                        }
                        auto currentExeInfo = QFileInfo(appFilePath);
                        auto currentExe = QFile(appFilePath);
                        if (currentExeInfo.isSymLink()) {
                            if (!currentExe.remove()) {
                                error = tr("Unable to remove current executable: %1").arg(currentExe.errorString());
                                return true;
                            }
                        } else {
                            const auto backupPath
                                = QString(currentExeInfo.path() % QChar('/') % currentExeInfo.baseName() % QStringLiteral("-backup-")
                                    % QString::fromUtf8(CppUtilities::applicationInfo.version) % QChar('.') % currentExeInfo.completeSuffix());
                            if (!currentExe.rename(backupPath)) {
                                error = tr("Unable to move current executable to \"%1\": %2").arg(backupPath, currentExe.errorString());
                                return true;
                            }
                        }
                        if (!newExe.link(currentExe.fileName())) {
                            error = tr("Unable to link new executable \"%1\" to \"%2\": %3")
                                        .arg(newExe.fileName(), currentExe.fileName(), newExe.errorString());
                            return true;
                        }
                        return true;
                    });
            } catch (const CppUtilities::ArchiveException &e) {
                error = tr("Unable to open downloaded archive: %1").arg(e.what());
            }
            if (error.isEmpty() && !foundExecutable) {
                error = tr("Unable to find executable in downloaded archive.");
            }
            return error;
        });
        m_p->watcher.setFuture(std::move(res));
    } break;
    default:
        setError(tr("Unable to download update: ") + reply->errorString());
    }
#endif
}

void Updater::handleUpdateStored()
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    m_p->error = m_p->watcher.result();
    if (!m_p->error.isEmpty()) {
        emit updateFailed(m_p->error);
        return;
    }
    emit updateStored();
#endif
}

struct UpdateHandlerPrivate {
    UpdateNotifier notifier;
    Updater updater = Updater(notifier.executableName());
    QTimer timer;
    QSettings *settings;
    std::optional<int> checkInterval;
};

UpdateHandler *UpdateHandler::s_mainInstance = nullptr;

/*!
 * \brief Handles checking for updates and performing an update of the application if available.
 * \todo Setup timer for regular checking and updater for performing update.
 */
UpdateHandler::UpdateHandler(QSettings *settings, QNetworkAccessManager *nm, QObject *parent)
    : QObject(parent)
    , m_p(std::make_unique<UpdateHandlerPrivate>())
{
    m_p->notifier.setNetworkAccessManager(nm);
    m_p->updater.setNetworkAccessManager(nm);
    m_p->timer.setSingleShot(true);
    m_p->timer.setTimerType(Qt::VeryCoarseTimer);
    m_p->settings = settings;
    connect(&m_p->timer, &QTimer::timeout, &m_p->notifier, &UpdateNotifier::checkForUpdate);
    connect(&m_p->updater, &Updater::updateStored, &m_p->notifier, &UpdateNotifier::resetUpdateInfo);
    connect(&m_p->notifier, &UpdateNotifier::checkedForUpdate, this, &UpdateHandler::handleUpdateCheckDone);
    applySettings();
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

int UpdateHandler::checkInterval() const
{
    if (m_p->checkInterval.has_value()) {
        return m_p->checkInterval.value();
    }
    m_p->settings->beginGroup(QStringLiteral("updating"));
    const auto i = m_p->settings->value("checkIntervalMs", 60 * 60 * 1000).toInt();
    m_p->settings->endGroup();
    return i;
}

void UpdateHandler::setCheckInterval(int checkInterval)
{
    m_p->checkInterval = checkInterval;
    m_p->settings->beginGroup(QStringLiteral("updating"));
    m_p->settings->setValue("checkIntervalMs", checkInterval);
    m_p->settings->endGroup();
    scheduleNextUpdateCheck();
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
    m_p->notifier.restore(m_p->settings);
    scheduleNextUpdateCheck();
}

void UpdateHandler::performUpdate()
{
    m_p->updater.performUpdate(m_p->notifier.downloadUrl().toString());
}

void UpdateHandler::handleUpdateCheckDone()
{
    m_p->notifier.save(m_p->settings);
    scheduleNextUpdateCheck();
}

void UpdateHandler::scheduleNextUpdateCheck()
{
    m_p->timer.stop();

    const auto intervalMs = checkInterval();
    if (intervalMs <= 0) {
        return;
    }
    const auto timeLeft = CppUtilities::TimeSpan::fromMilliseconds(intervalMs) - (CppUtilities::DateTime::now() - m_p->notifier.lastCheck());
    std::cerr << CppUtilities::EscapeCodes::Phrases::Info
              << "Check for updates due in: " << timeLeft.toString(CppUtilities::TimeSpanOutputFormat::WithMeasures)
              << CppUtilities::EscapeCodes::Phrases::End;
    m_p->timer.start(std::max(1000, static_cast<int>(timeLeft.totalMilliseconds())));
}

#ifdef QT_UTILITIES_GUI_QTWIDGETS
struct UpdateOptionPagePrivate {
    UpdateOptionPagePrivate(UpdateHandler &updateHandler)
        : updateHandler(updateHandler)
    {
    }
    UpdateHandler &updateHandler;
};

UpdateOptionPage::UpdateOptionPage(UpdateHandler &updateHandler, QWidget *parentWidget)
    : UpdateOptionPageBase(parentWidget)
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    , m_p(std::make_unique<UpdateOptionPagePrivate>(updateHandler))
#endif
{
}

UpdateOptionPage::~UpdateOptionPage()
{
}

bool UpdateOptionPage::apply()
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    m_p->updateHandler.setCheckInterval(ui()->checkIntervalSpinBox->value() * 60 * 1000);
#endif
    return true;
}

void UpdateOptionPage::reset()
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    ui()->checkIntervalSpinBox->setValue(m_p->updateHandler.checkInterval() / 60 / 1000);
#endif
}

QWidget *UpdateOptionPage::setupWidget()
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    // call base implementation first, so ui() is available
    if (m_p->updateHandler.notifier()->isSupported()) {
        auto *const widget = UpdateOptionPageBase::setupWidget();
        ui()->versionInUseValueLabel->setText(QString::fromUtf8(CppUtilities::applicationInfo.version));
        updateLatestVersion();
        QObject::connect(ui()->checkNowPushButton, &QPushButton::clicked, m_p->updateHandler.notifier(), &UpdateNotifier::checkForUpdate);
        QObject::connect(ui()->updatePushButton, &QPushButton::clicked, &m_p->updateHandler, &UpdateHandler::performUpdate);
        QObject::connect(
            m_p->updateHandler.notifier(), &UpdateNotifier::inProgressChanged, widget, [this](bool inProgress) { updateLatestVersion(inProgress); });
        QObject::connect(m_p->updateHandler.updater(), &Updater::updateFailed, widget, [this, widget](const QString &error) {
            QMessageBox::critical(widget, QCoreApplication::applicationName(),
                QCoreApplication::translate("QtUtilities::BuiltinWebViewOptionPage", "Unable to update: %1").arg(error));
        });
        return widget;
    }
#endif

    auto *const label = new QLabel;
    label->setWindowTitle(QCoreApplication::translate("QtGui::UpdateOptionPage", "Built-in web view"));
    label->setAlignment(Qt::AlignCenter);
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    label->setText(QCoreApplication::translate("QtUtilities::BuiltinWebViewOptionPage", "Checking for updates is not supported on this platform."));
#else
    label->setText(QCoreApplication::translate("QtUtilities::BuiltinWebViewOptionPage",
        "This build of %1 has automatic updates disabled. You may update the application in an automated way via your package manager, though.")
            .arg(CppUtilities::applicationInfo.name));
#endif
    return label;
}

void UpdateOptionPage::updateLatestVersion(bool)
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    ui()->latestVersionValueLabel->setText(m_p->updateHandler.notifier()->status());
    ui()->updatePushButton->setDisabled(m_p->updateHandler.notifier()->downloadUrl().isEmpty());
#endif
}

#endif

} // namespace QtUtilities
