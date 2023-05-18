#include "./updater.h"

#include "../misc/compat.h"
#include "../tests/mockreply.h"

#include "resources/config.h"

#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
#include <c++utilities/application/argumentparser.h>

#include <QDebug>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QStringBuilder>
#include <QVersionNumber>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
#define QT_UTILITIES_VERSION_SUFFIX QString()
#else
#define QT_UTILITIES_VERSION_SUFFIX QStringLiteral("-qt5")
#endif

namespace QtUtilities {

/*!
 * \brief
 */
void handleUpdate()
{
}

static QFile fileForRequest(const QString &dir, const QNetworkRequest &request)
{
    auto url = request.url();
    auto path = url.path();
    auto query = url.query();
    for (const auto c : {QChar('/'), QChar('&')}){
        path.replace(c, QChar('-'));
        query.replace(c, QChar('-'));
    }
    return QFile(dir % QChar('/') % url.host() % QChar('-') % path % QChar('-') % query % QStringLiteral(".txt"));
}

struct NetworkReplyData {
    explicit NetworkReplyData();
    ~NetworkReplyData();

    static NetworkReplyData current(QObject *sender);

    QNetworkReply *reply;
    QByteArray response;
};

NetworkReplyData::NetworkReplyData()
    : reply(nullptr)
{
}

NetworkReplyData::~NetworkReplyData()
{
    if (reply) {
        reply->deleteLater();
    }
}

NetworkReplyData NetworkReplyData::current(QObject *sender)
{
    // assign reply and read the response
    auto d = NetworkReplyData();
    d.reply = qobject_cast<QNetworkReply *>(sender);
    if (!d.reply) {
        qDebug() << "Update check: sender is no QNetworkReply";
        return d;
    }
    if (d.reply->error() == QNetworkReply::NoError) {
        d.response = d.reply->readAll();
    }

    // dump the response for debugging/testing if enabled
    static const auto dumpDir = qEnvironmentVariable(PROJECT_VARNAME_UPPER "_UPDATER_DUMP_RESPONSES");
    if (!dumpDir.isEmpty() && !qobject_cast<MockReply *>(sender)) {
        auto file = fileForRequest(dumpDir, d.reply->request());
        if (!file.open(QFile::WriteOnly) || !file.write(d.response)) {
            qDebug() << "Update check: Unable to dump response to '" << file.fileName() << "': " << file.errorString();
        }
        file.close();
    }

    return d;
}

struct UpdateNotifierPrivate {
    QNetworkAccessManager *nm = nullptr;
    QVersionNumber currentVersion = QVersionNumber();
    QRegularExpression gitHubRegex = QRegularExpression(QStringLiteral(".*/github.com/([^/]+)/([^/]+)(/.*)?"));
    QRegularExpression assetRegex = QRegularExpression();
    QString newVersion;
    QString additionalInfo;
    QString error;
    QUrl downloadUrl;
    QUrl releasesUrl;
    bool inProgress = false;
    bool updateAvailable = false;
    bool verbose = false;
};

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
    const auto gitHubMatch = m_p->gitHubRegex.match(url);
    const auto gitHubOrga = gitHubMatch.captured(1);
    const auto gitHubRepo = gitHubMatch.captured(2);
    if (gitHubOrga.isNull() || gitHubRepo.isNull()) {
        return;
    }
    const auto expectedBinaryName = gitHubRepo % QT_UTILITIES_VERSION_SUFFIX;

    m_p->releasesUrl = QStringLiteral("https://api.github.com/repos/") % gitHubOrga % QChar('/') % gitHubRepo % QStringLiteral("/releases?per_page=25");
    m_p->currentVersion = QVersionNumber::fromString(QUtf8StringView(appInfo.version));
#if defined(Q_OS_WIN64)
    m_p->assetRegex = QRegularExpression(expectedBinaryName + QStringLiteral("-.*-x86_64-w64-mingw32\\.exe\\..+"));
#elif defined(Q_OS_WIN32)
    m_p->assetRegex = QRegularExpression(expectedBinaryName + QStringLiteral("-.*-i686-w64-mingw32\\.exe\\..+"));
#elif defined(__GNUC__) && defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) && defined(__x86_64__)
    m_p->assetRegex = QRegularExpression(expectedBinaryName + QStringLiteral("-.*-x86_64-pc-linux-gnu\\..+"));
#endif
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

const QString &UpdateNotifier::newVersion() const
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    static const auto v = QString();
    return v;
#else
    return m_p->newVersion;
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

void UpdateNotifier::setNetworkAccessManager(QNetworkAccessManager *nm)
{
#ifndef QT_UTILITIES_SETUP_TOOLS_ENABLED
    Q_UNUSED(nm)
#else
    m_p->nm = nm;
#endif
}

void UpdateNotifier::setError(const QString &context, QNetworkReply *reply)
{
    m_p->error = context + reply->errorString();
    emit inProgressChanged(m_p->inProgress = false);
}

void UpdateNotifier::setError(const QString &context, const QJsonParseError &jsonError, const QByteArray &response, QNetworkReply *)
{
    m_p->error = context % jsonError.errorString() % QChar(' ') % QChar('(') % tr("at offset %1").arg(jsonError.offset) % QChar(')');
    if (!response.isEmpty()) {
        m_p->error += QStringLiteral("\nResponse was: ");
        m_p->error += QString::fromUtf8(response);
    }
    emit inProgressChanged(m_p->inProgress = false);
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
    auto *const reply = makeRequest(QNetworkRequest(m_p->releasesUrl));
    connect(reply, &QNetworkReply::finished, this, &UpdateNotifier::readReleases);
#endif
}

void UpdateNotifier::readReleases()
{
    const auto d = NetworkReplyData::current(sender());
    switch (d.reply->error()) {
    case QNetworkReply::NoError: {
        // parse JSON
        auto jsonError = QJsonParseError();
        const auto replyDoc = QJsonDocument::fromJson(d.response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            setError(tr("Unable to parse releases: "), jsonError, d.response, d.reply);
            return;
        }
#if !defined(QT_JSON_READONLY)
        if (m_p->verbose) {
            qDebug().noquote() << "Update check: found releases: " << QString::fromUtf8(replyDoc.toJson(QJsonDocument::Indented));
        }
#endif
        const auto replyArray = replyDoc.array();
        for (const auto &releaseInfoVal : replyArray) {
            const auto releaseInfo = releaseInfoVal.toObject();
            const auto tag = releaseInfo.value(QLatin1String("tag_name")).toString();
            const auto version = QVersionNumber::fromString(tag.startsWith(QChar('v')) ? tag.mid(1) : tag);
            if (!version.isNull() && version > m_p->currentVersion) {
                const auto assets = releaseInfo.value(QLatin1String("assets"));
                m_p->newVersion = version.toString();
                if (assets.isArray()) {
                    processAssets(assets.toArray());
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
        break;
    }
    case QNetworkReply::OperationCanceledError:
        emit inProgressChanged(m_p->inProgress = false);
        return;
    default:
        setError(tr("Unable to request releases: "), d.reply);
    }
}

void UpdateNotifier::queryRelease(const QUrl &releaseUrl)
{
    auto *const reply = makeRequest(QNetworkRequest(releaseUrl));
    connect(reply, &QNetworkReply::finished, this, &UpdateNotifier::readRelease);
}

void UpdateNotifier::readRelease()
{
    const auto d = NetworkReplyData::current(sender());
    switch (d.reply->error()) {
    case QNetworkReply::NoError: {
        // parse JSON
        auto jsonError = QJsonParseError();
        const auto replyDoc = QJsonDocument::fromJson(d.response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            setError(tr("Unable to parse release: "), jsonError, d.response, d.reply);
            return;
        }
#if !defined(QT_JSON_READONLY)
        if (m_p->verbose) {
            qDebug().noquote() << "Update check: found release info: " << QString::fromUtf8(replyDoc.toJson(QJsonDocument::Indented));
        }
#endif
        processAssets(replyDoc.object().value(QLatin1String("assets")).toArray());
        emit inProgressChanged(m_p->inProgress = false);
        break;
    }
    case QNetworkReply::OperationCanceledError:
        emit inProgressChanged(m_p->inProgress = false);
        return;
    default:
        setError(tr("Unable to request release: "), d.reply);
    }
}

void UpdateNotifier::processAssets(const QJsonArray &assets)
{
    for (const auto &assetVal : assets) {
        const auto asset = assetVal.toObject();
        const auto assetName = asset.value(QLatin1String("name")).toString();
        if (!assetName.isEmpty() && m_p->assetRegex.match(assetName).hasMatch()) {
            m_p->downloadUrl = asset.value(QLatin1String("browser_download_url")).toString();
            m_p->updateAvailable = true;
            emit updateAvailable(m_p->newVersion, m_p->additionalInfo);
            return;
        }
        if (m_p->verbose) {
            qDebug() << "Update check: skipping asset: " << assetName;
        }
    }
}

QNetworkReply *UpdateNotifier::makeRequest(const QNetworkRequest &request)
{
    static const auto mockDir = qEnvironmentVariable(PROJECT_VARNAME_UPPER "_UPDATER_MOCK_RESPONSES");
    if (m_p->nm && mockDir.isEmpty()) {
        return m_p->nm->get(request);
    } else {
        // mock the response for debugging/testing if enabled
        auto file = fileForRequest(mockDir, request);
        qDebug() << "Update check: loading mock response from: " << file.fileName();
        return MockReply::forFile(request, std::move(file));
    }
}

} // namespace QtUtilities
