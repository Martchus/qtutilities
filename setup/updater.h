#ifndef QT_UTILITIES_SETUP_UPDATER_H
#define QT_UTILITIES_SETUP_UPDATER_H

#include "../global.h"
#include "../settingsdialog/optionpage.h" // TODO: UpdateOptionPage

#include <QObject>
#include <QUrl>

#include <memory>

QT_FORWARD_DECLARE_CLASS(QJsonParseError)
QT_FORWARD_DECLARE_CLASS(QJsonArray)
QT_FORWARD_DECLARE_CLASS(QNetworkAccessManager)
QT_FORWARD_DECLARE_CLASS(QNetworkReply)

namespace QtUtilities {

struct UpdateNotifierPrivate;

class QT_UTILITIES_EXPORT UpdateNotifier : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool supported READ isSupported)
    Q_PROPERTY(bool inProgress READ isInProgress NOTIFY inProgressChanged)
    Q_PROPERTY(bool updateAvailable READ isUpdateAvailable)
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
    const QString &newVersion() const;
    const QString &additionalInfo() const;
    const QString &error() const;
    const QUrl &downloadUrl() const;
    void setNetworkAccessManager(QNetworkAccessManager *nm);

public Q_SLOTS:
    void checkForUpdate();

Q_SIGNALS:
    void inProgressChanged(bool inProgress);
    void updateAvailable(const QString &version, const QString &additionalInfo);

private Q_SLOTS:
    void setError(const QString &context, QNetworkReply *reply);
    void setError(const QString &context, const QJsonParseError &jsonError, const QByteArray &response, QNetworkReply *reply);
    void readReleases();
    void queryRelease(const QUrl &releaseUrl);
    void readRelease();
    void processAssets(const QJsonArray &assets);

private:
    std::unique_ptr<UpdateNotifierPrivate> m_p;
};

struct UpdaterPrivate;

class QT_UTILITIES_EXPORT Updater : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString error READ error)

public:
    explicit Updater(QObject *parent = nullptr);
    ~Updater() override;

    const QString &error() const;
    void setNetworkAccessManager(QNetworkAccessManager *nm);

public Q_SLOTS:
    void performUpdate(const QString &downloadUrl);

Q_SIGNALS:
    void updateFailed(const QString &error);
    void updateStored();

private Q_SLOTS:
    void setError(const QString &error);
    void startDownload(const QString &downloadUrl);
    void storeExecutable();
    void restartApplication();

private:
    std::unique_ptr<UpdaterPrivate> m_p;
};

// TODO: UpdateWidget, UpdateOptionPage (wraps UpdateWidget for use in SettingsDialog), UpdateConfig (entry point, loads settings and "last checked" via QSettings)

QT_UTILITIES_EXPORT UpdateNotifier *handleUpdate(QNetworkAccessManager *nm, QObject *parent = nullptr);

} // namespace QtUtilities

#endif // QT_UTILITIES_SETUP_UPDATER_H
