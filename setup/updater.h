#ifndef QT_UTILITIES_SETUP_UPDATER_H
#define QT_UTILITIES_SETUP_UPDATER_H

#include "../global.h"

#ifdef QT_UTILITIES_GUI_QTWIDGETS
#include "../settingsdialog/optionpage.h"
#include "../settingsdialog/settingsdialog.h"
#endif

#include <c++utilities/chrono/datetime.h>
#include <c++utilities/misc/flagenumclass.h>

#include <QObject>
#include <QUrl>

#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
#include <QNetworkRequest>
#endif
#ifdef QT_UTILITIES_GUI_QTWIDGETS
#include <QMessageBox>
#endif

#include <atomic>
#include <memory>

QT_FORWARD_DECLARE_CLASS(QJsonParseError)
QT_FORWARD_DECLARE_CLASS(QJsonArray)
QT_FORWARD_DECLARE_CLASS(QNetworkAccessManager)
QT_FORWARD_DECLARE_CLASS(QNetworkReply)
QT_FORWARD_DECLARE_CLASS(QSettings)

namespace QtUtilities {

/// \cond
struct UpdateNotifierPrivate;
struct UpdaterPrivate;
struct UpdateHandlerPrivate;
struct UpdateOptionPagePrivate;
struct UpdateDialogPrivate;
/// \endcond

enum class UpdateCheckFlags : quint64 {
    None = 0,
    IncludePreReleases = (1 << 0),
    IncludeDrafts = (1 << 1),
    Default = None,
};

} // namespace QtUtilities

CPP_UTILITIES_MARK_FLAG_ENUM_CLASS(QtUtilities, QtUtilities::UpdateCheckFlags);

namespace QtUtilities {

/// \brief The UpdateNotifier class allows checking for new updates.
/// \remarks This class is experimental and might be changed in incompatible ways (API and ABI wise) or completely removed
/// in further minor/patch releases.
class QT_UTILITIES_EXPORT UpdateNotifier : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool supported READ isSupported)
    Q_PROPERTY(bool inProgress READ isInProgress NOTIFY inProgressChanged)
    Q_PROPERTY(bool updateAvailable READ isUpdateAvailable)
    Q_PROPERTY(QString executableName READ executableName CONSTANT)
    Q_PROPERTY(QString newVersion READ newVersion)
    Q_PROPERTY(QString additionalInfo READ additionalInfo)
    Q_PROPERTY(QString releaseNotes READ releaseNotes)
    Q_PROPERTY(QString error READ error)
    Q_PROPERTY(QUrl downloadUrl READ downloadUrl)
    Q_PROPERTY(QUrl signatureUrl READ signatureUrl)

public:
    explicit UpdateNotifier(QObject *parent = nullptr);
    ~UpdateNotifier() override;

    bool isSupported() const;
    bool isInProgress() const;
    bool isUpdateAvailable() const;
    UpdateCheckFlags flags() const;
    void setFlags(UpdateCheckFlags flags);
    const QString &executableName() const;
    const QString &newVersion() const;
    const QString &latestVersion() const;
    const QString &additionalInfo() const;
    const QString &releaseNotes() const;
    const QString &error() const;
    const QUrl &downloadUrl() const;
    const QUrl &signatureUrl() const;
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
    void supplyNewReleaseData(const QByteArray &data);

Q_SIGNALS:
    void inProgressChanged(bool inProgress);
    void checkedForUpdate();
    void updateAvailable(const QString &version, const QString &additionalInfo);

private Q_SLOTS:
    void lastCheckNow() const;
    void setError(const QString &context, QNetworkReply *reply);
    void setError(const QString &context, const QJsonParseError &jsonError, const QByteArray &response);
    void readReleases();
    void queryRelease(const QUrl &releaseUrl);
    void readRelease();
    void processAssets(const QJsonArray &assets, bool forUpdate);

private:
    std::unique_ptr<UpdateNotifierPrivate> m_p;
};

/// \brief The Updater class allows downloading and applying an update.
/// \remarks This class is experimental and might be changed in incompatible ways (API and ABI wise) or completely removed
/// in further minor/patch releases.
class QT_UTILITIES_EXPORT Updater : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool inProgress READ isInProgress NOTIFY inProgressChanged)
    Q_PROPERTY(QString overallStatus READ overallStatus NOTIFY inProgressChanged)
    Q_PROPERTY(QString error READ error NOTIFY updateStatusChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY updateStatusChanged)
    Q_PROPERTY(QString storedPath READ storedPath NOTIFY inProgressChanged)

public:
    struct Update {
        std::string_view executableName;
        std::string_view signatureName;
        std::string_view data;
        std::string_view signature;
    };
    using VerifyFunction = std::function<QString(const Update &)>;

    explicit Updater(const QString &executableName, QObject *parent = nullptr);
    explicit Updater(const QString &executableName, const QString &signatureExtension, QObject *parent = nullptr);
    ~Updater() override;

    bool isInProgress() const;
    QString overallStatus() const;
    const QString &error() const;
    const QString &statusMessage() const;
    const QString &storedPath() const;
    void setNetworkAccessManager(QNetworkAccessManager *nm);
    void setVerifier(VerifyFunction &&verifyFunction);
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    void setCacheLoadControl(QNetworkRequest::CacheLoadControl cacheLoadControl);
#endif

public Q_SLOTS:
    bool performUpdate(const QString &downloadUrl, const QString &signatureUrl);
    void abortUpdate();

Q_SIGNALS:
    void inProgressChanged(bool inProgress);
    void updateFailed(const QString &error);
    void updateStored();
    void updatePercentageChanged(qint64 bytesReceived, qint64 bytesTotal);
    void updateStatusChanged(const QString &statusMessage);

private Q_SLOTS:
    void setError(const QString &error);
    void startDownload(const QString &downloadUrl, const QString &signatureUrl);
    void handleDownloadFinished();
    void readSignature();
    void storeExecutable();
    void concludeUpdate();

private:
    std::unique_ptr<UpdaterPrivate> m_p;
};

/// \brief The UpdateHandler class manages the non-graphical aspects of checking for new updates and performing them.
/// \remarks This class is experimental and might be changed in incompatible ways (API and ABI wise) or completely removed
/// in further minor/patch releases.
class QT_UTILITIES_EXPORT UpdateHandler : public QObject {
    Q_OBJECT
    Q_PROPERTY(UpdateNotifier *notifier READ notifier CONSTANT)
    Q_PROPERTY(Updater *updater READ updater CONSTANT)

public:
    /// \brief The CheckInterval struct specifies whether automatic checks for updates are enabled and of often they should be done.
    struct CheckInterval {
        /// \brief The duration of the interval. Only durations up to around 24 days are supported. Only full-second accuracy is supported.
        /// \remarks A value of zero indicates that the check is supposed to be done only once every time the application launches (i.e. shortly after startup).
        CppUtilities::TimeSpan duration;
        /// \brief Whether automatic checks for updates are enabled at all.
        bool enabled = true;
    };

    explicit UpdateHandler(QSettings *settings, QNetworkAccessManager *nm, QObject *parent = nullptr);
    explicit UpdateHandler(
        const QString &executableName, const QString &signatureExtension, QSettings *settings, QNetworkAccessManager *nm, QObject *parent = nullptr);
    ~UpdateHandler() override;

    UpdateNotifier *notifier();
    Updater *updater();
    const CheckInterval &checkInterval() const;
    void setCheckInterval(CheckInterval checkInterval);
    bool isConsideringSeparateSignature() const;
    void setConsideringSeparateSignature(bool consideringSeparateSignature);
    QString preCheck() const;
    static UpdateHandler *mainInstance();
    static void setMainInstance(UpdateHandler *mainInstance);
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    void setCacheLoadControl(QNetworkRequest::CacheLoadControl cacheLoadControl);
#endif

public Q_SLOTS:
    void applySettings();
    void performUpdate();
    void saveNotifierState();

private Q_SLOTS:
    void handleUpdateCheckDone();

private:
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    void scheduleNextUpdateCheck();
#endif

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

/// \brief The RestartHandler class allows quitting and respawning the application if a restart is requested.
/// \remarks This class is experimental and might be changed in incompatible ways (API and ABI wise) or completely removed
/// in further minor/patch releases.
class QT_UTILITIES_EXPORT RestartHandler {
public:
    explicit RestartHandler()
        : m_restartRequested(false)
    {
    }
    bool isRestartRequested()
    {
        return m_restartRequested;
    }
    void requestRestart();
    void reset()
    {
        m_restartRequested = false;
    }
    void respawnIfRestartRequested();
    std::function<void()> requester()
    {
        return [this] { requestRestart(); };
    }

private:
    std::atomic_bool m_restartRequested;
};

#ifdef QT_UTILITIES_GUI_QTWIDGETS
/// \brief The UpdateOptionPage class provides a settings page to manage automatic updates for applications using Qt Widgets.
/// \remarks This class is experimental and might be changed in incompatible ways (API and ABI wise) or completely removed
/// in further minor/patch releases.
BEGIN_DECLARE_UI_FILE_BASED_OPTION_PAGE_CUSTOM_CTOR(UpdateOptionPage)
public:
explicit UpdateOptionPage(UpdateHandler *updateHandler, QWidget *parentWidget = nullptr);

void setRestartHandler(std::function<void()> &&handler);

private:
DECLARE_SETUP_WIDGETS
void updateLatestVersion(bool inProgress = false);
std::unique_ptr<UpdateOptionPagePrivate> m_p;
END_DECLARE_OPTION_PAGE

/// \brief The VerificationErrorMessageBox class provides message box for showing signature validation errors during updates.
/// \remarks This class is experimental and might be changed in incompatible ways (API and ABI wise) or completely removed
/// in further minor/patch releases.
class QT_UTILITIES_EXPORT VerificationErrorMessageBox : public QMessageBox {
    Q_OBJECT

public:
    explicit VerificationErrorMessageBox();
    ~VerificationErrorMessageBox() override;

public Q_SLOTS:
    int execForError(QString &errorMessage, const QString &explanation = QString());
    void openForError(const QString &errorMessage, const QString &explanation = QString());
};

/// \brief The UpdateDialog class provides a settings dialog to manage automatic updates for applications using Qt Widgets.
/// \remarks This class is experimental and might be changed in incompatible ways (API and ABI wise) or completely removed
/// in further minor/patch releases.
class QT_UTILITIES_EXPORT UpdateDialog : public SettingsDialog {
    Q_OBJECT

public:
    explicit UpdateDialog(QWidget *parent = nullptr);
    ~UpdateDialog() override;
    UpdateOptionPage *page();
    const UpdateOptionPage *page() const;

private:
    std::unique_ptr<UpdateDialogPrivate> m_p;
};

#endif

} // namespace QtUtilities

#endif // QT_UTILITIES_SETUP_UPDATER_H

#if defined(QT_UTILITIES_GUI_QTWIDGETS)
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE(UpdateOptionPage)
#endif
