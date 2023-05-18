#ifndef QT_UTILITIES_SETUP_UPDATER_H
#define QT_UTILITIES_SETUP_UPDATER_H

#include "../global.h"

#ifdef QT_UTILITIES_GUI_QTWIDGETS
#include "../settingsdialog/optionpage.h"
#endif

#include <c++utilities/chrono/datetime.h>

#include <QObject>
#include <QUrl>

#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
#include <QNetworkRequest>
#endif

#include <memory>

QT_FORWARD_DECLARE_CLASS(QJsonParseError)
QT_FORWARD_DECLARE_CLASS(QJsonArray)
QT_FORWARD_DECLARE_CLASS(QNetworkAccessManager)
QT_FORWARD_DECLARE_CLASS(QNetworkReply)
QT_FORWARD_DECLARE_CLASS(QSettings)

namespace QtUtilities {

struct UpdateNotifierPrivate;

class QT_UTILITIES_EXPORT UpdateNotifier : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool supported READ isSupported)
    Q_PROPERTY(bool inProgress READ isInProgress NOTIFY inProgressChanged)
    Q_PROPERTY(bool updateAvailable READ isUpdateAvailable)
    Q_PROPERTY(QString executableName READ executableName CONSTANT)
    Q_PROPERTY(QString newVersion READ newVersion)
    Q_PROPERTY(QString additionalInfo READ additionalInfo)
    Q_PROPERTY(QString error READ error)
    Q_PROPERTY(QUrl downloadUrl READ downloadUrl)

public:
    explicit UpdateNotifier(QObject *parent = nullptr);
    ~UpdateNotifier() override;

    bool isSupported() const;
    bool isInProgress() const;
    bool isUpdateAvailable() const;
    const QString &executableName() const;
    const QString &newVersion() const;
    const QString &latestVersion() const;
    const QString &additionalInfo() const;
    const QString &error() const;
    const QUrl &downloadUrl() const;
    QString status() const;
    CppUtilities::DateTime lastCheck() const;
    void restore(QSettings *settings);
    void save(QSettings *settings);
    void setNetworkAccessManager(QNetworkAccessManager *nm);
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    void setCacheLoadControl(QNetworkRequest::CacheLoadControl cacheLoadControl);
#endif

public Q_SLOTS:
    void checkForUpdate();
    void resetUpdateInfo();

Q_SIGNALS:
    void inProgressChanged(bool inProgress);
    void checkedForUpdate();
    void updateAvailable(const QString &version, const QString &additionalInfo);

private Q_SLOTS:
    void lastCheckNow() const;
    void setError(const QString &context, QNetworkReply *reply);
    void setError(const QString &context, const QJsonParseError &jsonError, const QByteArray &response, QNetworkReply *reply);
    void readReleases();
    void queryRelease(const QUrl &releaseUrl);
    void readRelease();
    void processAssets(const QJsonArray &assets, bool forUpdate);

private:
    std::unique_ptr<UpdateNotifierPrivate> m_p;
};

struct UpdaterPrivate;

class QT_UTILITIES_EXPORT Updater : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString error READ error)

public:
    explicit Updater(const QString &executableName, QObject *parent = nullptr);
    ~Updater() override;

    const QString &error() const;
    void setNetworkAccessManager(QNetworkAccessManager *nm);
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    void setCacheLoadControl(QNetworkRequest::CacheLoadControl cacheLoadControl);
#endif

public Q_SLOTS:
    void performUpdate(const QString &downloadUrl);

Q_SIGNALS:
    void updateFailed(const QString &error);
    void updateStored();

private Q_SLOTS:
    void setError(const QString &error);
    void startDownload(const QString &downloadUrl);
    void storeExecutable();
    void handleUpdateStored();

private:
    std::unique_ptr<UpdaterPrivate> m_p;
};

struct UpdateHandlerPrivate;

class QT_UTILITIES_EXPORT UpdateHandler : public QObject {
    Q_OBJECT
    Q_PROPERTY(UpdateNotifier *notifier READ notifier CONSTANT)
    Q_PROPERTY(Updater *updater READ updater CONSTANT)
    Q_PROPERTY(int checkInterval READ checkInterval WRITE setCheckInterval)

public:
    explicit UpdateHandler(QSettings *settings, QNetworkAccessManager *nm, QObject *parent = nullptr);
    ~UpdateHandler() override;

    UpdateNotifier *notifier();
    Updater *updater();
    int checkInterval() const;
    void setCheckInterval(int checkInterval);
    static UpdateHandler *mainInstance();
    static void setMainInstance(UpdateHandler *mainInstance);
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    void setCacheLoadControl(QNetworkRequest::CacheLoadControl cacheLoadControl);
#endif

public Q_SLOTS:
    void applySettings();
    void performUpdate();

private Q_SLOTS:
    void handleUpdateCheckDone();
    void scheduleNextUpdateCheck();

private:
    std::unique_ptr<UpdateHandlerPrivate> m_p;
    static UpdateHandler *s_mainInstance;
};

inline UpdateHandler *UpdateHandler::mainInstance()
{
    return s_mainInstance;
}

inline void UpdateHandler::setMainInstance(UpdateHandler *mainInstance)
{
    s_mainInstance = mainInstance;
}

#ifdef QT_UTILITIES_GUI_QTWIDGETS

struct UpdateOptionPagePrivate;

BEGIN_DECLARE_UI_FILE_BASED_OPTION_PAGE_CUSTOM_CTOR(UpdateOptionPage)
public:
explicit UpdateOptionPage(UpdateHandler &updateHandler, QWidget *parentWidget = nullptr);

private:
DECLARE_SETUP_WIDGETS
void updateLatestVersion(bool inProgress = false);
std::unique_ptr<UpdateOptionPagePrivate> m_p;
END_DECLARE_OPTION_PAGE

#endif

} // namespace QtUtilities

#endif // QT_UTILITIES_SETUP_UPDATER_H
