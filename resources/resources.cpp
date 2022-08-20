#include "./resources.h"

#include "resources/config.h"

#include <QDir>
#include <QFile>
#include <QFont>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <QSettings>
#include <QString>
#include <QStringBuilder>
#include <QTranslator>
#if defined(QT_UTILITIES_GUI_QTWIDGETS)
#include <QApplication>
#elif defined(QT_UTILITIES_GUI_QTQUICK)
#include <QGuiApplication>
#else
#include <QCoreApplication>
#endif

#include <iostream>

using namespace std;

///! \cond
inline void initResources()
{
    Q_INIT_RESOURCE(qtutilsicons);
}

inline void cleanupResources()
{
    Q_CLEANUP_RESOURCE(qtutilsicons);
}
///! \endcond

namespace QtUtilities {

/*!
 * \brief Functions for using the resources provided by this library.
 * \deprecated Replaced by ENABLE_QT_RESOURCES_OF_STATIC_DEPENDENCIES macro.
 */
namespace QtUtilitiesResources {

/*!
 * \brief Initiates the resources used and provided by this library.
 * \deprecated Replaced by ENABLE_QT_RESOURCES_OF_STATIC_DEPENDENCIES macro.
 */
void init()
{
    initResources();
}

/*!
 * \brief Frees the resources used and provided by this library.
 * \deprecated Replaced by ENABLE_QT_RESOURCES_OF_STATIC_DEPENDENCIES macro.
 */
void cleanup()
{
    cleanupResources();
}
} // namespace QtUtilitiesResources

/*!
 * \brief Convenience functions to load translations for Qt and the application.
 */
namespace TranslationFiles {

/*!
 * \brief Allows to set an additional search path for translation files.
 * \remarks This path is considered *before* the default directories.
 */
QString &additionalTranslationFilePath()
{
    static QString path;
    return path;
}

/*!
 * \brief Loads and installs the appropriate Qt translation file for the current
 * locale.
 * \param repositoryNames Specifies the names of the Qt repositories to load
 * translations for (eg. qtbase, qtscript, ...).
 * \remarks
 *  - Translation files have to be placed in one of the following locations:
 *    * QLibraryInfo::location(QLibraryInfo::TranslationsPath) (used in UNIX)
 *    * ../share/qt/translations (used in Windows)
 *  - Translation files can also be built-in using by setting the CMake variable
 *    BUILTIN_TRANSLATIONS.
 *    In this case it is also necessary to load the translations using this
 *    function.
 */
void loadQtTranslationFile(std::initializer_list<QString> repositoryNames)
{
    loadQtTranslationFile(repositoryNames, QLocale().name());
}

/*!
 * \brief Loads and installs the appropriate Qt translation file for the
 * specified locale.
 * \param repositoryNames Specifies the names of the Qt repositories to load
 * translations for (eg. qtbase, qtscript, ...).
 * \param localeName Specifies the name of the locale.
 * \remarks
 *  - Translation files have to be placed in one of the following locations:
 *    * QLibraryInfo::location(QLibraryInfo::TranslationsPath) (used in UNIX)
 *    * ../share/qt/translations (used in Windows)
 *  - Translation files can also be built-in using by setting the CMake variable
 *    BUILTIN_TRANSLATIONS.
 *    In this case it is also necessary to load the translations using this
 *    function.
 */
void loadQtTranslationFile(initializer_list<QString> repositoryNames, const QString &localeName)
{
    const auto debugTranslations = qEnvironmentVariableIsSet("QT_DEBUG_TRANSLATIONS");
    for (const auto &repoName : repositoryNames) {
        auto *const qtTranslator = new QTranslator(QCoreApplication::instance());
        const auto fileName = QString(repoName % QChar('_') % localeName);

        QString path;
        if ((!additionalTranslationFilePath().isEmpty() && qtTranslator->load(fileName, path = additionalTranslationFilePath()))
            || qtTranslator->load(fileName,
                path =
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
                    QLibraryInfo::location(QLibraryInfo::TranslationsPath)
#else
                    QLibraryInfo::path(QLibraryInfo::TranslationsPath)
#endif
                    )
            || qtTranslator->load(fileName, path = QStringLiteral("../share/qt/translations"))
            || qtTranslator->load(fileName, path = QStringLiteral(":/translations"))) {
            QCoreApplication::installTranslator(qtTranslator);
            if (debugTranslations) {
                cerr << "Loading translation file for Qt repository \"" << repoName.toLocal8Bit().data() << "\" and the locale \""
                     << localeName.toLocal8Bit().data() << "\" from \"" << path.toLocal8Bit().data() << "\"." << endl;
            }
        } else {
            delete qtTranslator;
            if (localeName.startsWith(QLatin1String("en"))) {
                // the translation file is probably just empty (English is built-in and usually only used for plural forms)
                continue;
            }
            cerr << "Unable to load translation file for Qt repository \"" << repoName.toLocal8Bit().data() << "\" and locale "
                 << localeName.toLocal8Bit().data() << "." << endl;
        }
    }
}

/*!
 * \brief Loads and installs the appropriate application translation file for
 * the current locale.
 * \param applicationName Specifies the name of the application.
 * \remarks
 *  - Translation files have to be placed in one of the following locations:
 *    * ./
 *    * ../$application
 *    * ../../$application
 *    * ./translations
 *    * ../share/$application/translations
 *    * $install_prefix/share/$application/translations
 *  - Translation files must be named using the following scheme:
 *    * $application_$language.qm
 *  - Translation files can also be built-in using by setting the CMake variable
 *    BUILTIN_TRANSLATIONS.
 *    In this case it is also necessary to load the translations using this
 *    function.
 */
void loadApplicationTranslationFile(const QString &configName, const QString &applicationName)
{
    // load English translation files as fallback
    loadApplicationTranslationFile(configName, applicationName, QStringLiteral("en_US"));
    // load translation files for current locale
    const auto defaultLocale(QLocale().name());
    if (defaultLocale != QLatin1String("en_US")) {
        loadApplicationTranslationFile(configName, applicationName, defaultLocale);
    }
}

/// \cond
void logTranslationEvent(
    const char *event, const QString &configName, const QString &applicationName, const QString &localeName, const QString &path = QString())
{
    cerr << event << " translation file for \"" << applicationName.toLocal8Bit().data() << "\"";
    if (!configName.isEmpty()) {
        cerr << " (config \"" << configName.toLocal8Bit().data() << "\")";
    }
    cerr << " and locale \"" << localeName.toLocal8Bit().data() << '\"';
    if (!path.isEmpty()) {
        cerr << " from \"" << path.toLocal8Bit().data() << '\"';
    }
    cerr << '.' << endl;
}
/// \endcond

/*!
 * \brief Loads and installs the appropriate application translation file for
 * the specified locale.
 * \param applicationName Specifies the name of the application.
 * \param localeName Specifies the name of the locale.
 * \remarks
 *  - Translation files have to be placed in one of the following locations:
 *    * ./
 *    * ../$application
 *    * ../../$application
 *    * ./translations
 *    * ../share/$application/translations
 *    * $install_prefix/share/$application/translations
 *  - Translation files must be named using the following scheme:
 *    * $application_$language.qm
 *  - Translation files can also be built-in using by setting the CMake variable
 *    BUILTIN_TRANSLATIONS.
 *    In this case it is also necessary to load the translations using this
 *    function.
 */
void loadApplicationTranslationFile(const QString &configName, const QString &applicationName, const QString &localeName)
{
    auto *const appTranslator = new QTranslator(QCoreApplication::instance());
    const auto fileName = QString(applicationName % QChar('_') % localeName);
    const auto directoryName = configName.isEmpty() ? applicationName : QString(applicationName % QChar('-') % configName);

    QString path;
    if ((!additionalTranslationFilePath().isEmpty() && appTranslator->load(fileName, path = additionalTranslationFilePath()))
        || appTranslator->load(fileName, path = QStringLiteral(".")) || appTranslator->load(fileName, path = QStringLiteral("../") % directoryName)
        || appTranslator->load(fileName, path = QStringLiteral("../") % directoryName)
        || appTranslator->load(fileName, path = QStringLiteral("../../") % directoryName)
        || appTranslator->load(fileName, path = QStringLiteral("./translations"))
        || appTranslator->load(fileName, path = QStringLiteral("../share/") % directoryName % QStringLiteral("/translations"))
        || appTranslator->load(fileName, path = QStringLiteral(APP_INSTALL_PREFIX "/share/") % directoryName % QStringLiteral("/translations"))
        || appTranslator->load(fileName, path = QStringLiteral(":/translations"))) {
        QCoreApplication::installTranslator(appTranslator);
        if (qEnvironmentVariableIsSet("QT_DEBUG_TRANSLATIONS")) {
            logTranslationEvent("Loading", configName, applicationName, localeName, path);
        }
    } else {
        delete appTranslator;
        if (localeName.startsWith(QLatin1String("en"))) {
            // the translation file is probably just empty (English is built-in and usually only used for plural forms)
            return;
        }
        logTranslationEvent("Unable to load", configName, applicationName, localeName);
    }
}

/*!
 * \brief Loads and installs the appropriate application translation file for
 * the current locale.
 * \param applicationNames Specifies the names of the applications.
 */
void loadApplicationTranslationFile(const QString &configName, const std::initializer_list<QString> &applicationNames)
{
    for (const QString &applicationName : applicationNames) {
        loadApplicationTranslationFile(configName, applicationName);
    }
}

/*!
 * \brief Loads and installs the appropriate application translation file for
 * the specified locale.
 * \param applicationNames Specifies the names of the applications.
 * \param localeName Specifies the name of the locale.
 */
void loadApplicationTranslationFile(const QString &configName, const std::initializer_list<QString> &applicationNames, const QString &localeName)
{
    for (const QString &applicationName : applicationNames) {
        loadApplicationTranslationFile(configName, applicationName, localeName);
    }
}
} // namespace TranslationFiles

/*!
 * \brief Convenience functions to check whether a
 * QCoreApplication/QGuiApplication/QApplication singleton has been instantiated
 * yet.
 */
namespace ApplicationInstances {

#if defined(QT_UTILITIES_GUI_QTWIDGETS)
/*!
 * \brief Returns whether a QApplication has been instantiated yet.
 */
bool hasWidgetsApp()
{
    return qobject_cast<QApplication *>(QCoreApplication::instance()) != nullptr;
}
#endif

#if defined(QT_UTILITIES_GUI_QTWIDGETS) || defined(QT_UTILITIES_GUI_QTQUICK)
/*!
 * \brief Returns whether a QGuiApplication has been instantiated yet.
 */
bool hasGuiApp()
{
    return qobject_cast<QGuiApplication *>(QCoreApplication::instance()) != nullptr;
}
#endif

/*!
 * \brief Returns whether a QCoreApplication has been instantiated yet.
 */
bool hasCoreApp()
{
    return qobject_cast<QCoreApplication *>(QCoreApplication::instance()) != nullptr;
}
} // namespace ApplicationInstances

/*!
 * \brief Sets Qt application attributes which are commonly used within my Qt applications.
 * \remarks
 * - So far this enables High-DPI support.
 * - The exact attributes are unspecified and might change to whatever makes sense in the future.
 */
void setupCommonQtApplicationAttributes()
{
#ifdef QT_FEATURE_fontdialog
    if (!qEnvironmentVariableIsSet("FONTCONFIG_PATH") && QDir(QStringLiteral("/etc/fonts")).exists()) {
        qputenv("FONTCONFIG_PATH", "/etc/fonts");
    }
#endif
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    if (!QCoreApplication::instance()) {
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    }
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
#endif
}

/*!
 * \brief Returns the settings object for the specified \a organization and \a application.
 * \remarks
 * - This function always uses INI as that's what I'd like to use in all of my applications consistently, regardless of the platform.
 * - The parameter \a application might be empty. In fact, most of my applications use just `getSettings(QStringLiteral(PROJECT_NAME))`.
 * - This function first checks whether a file called `$organization/$application.ini` exists in the current working directory (or just
 *   `$organization.ini` if \a application is empty) and uses that if it exists. That allows having a portable installation.
 * - Some of my apps where using values from QCoreApplication for \a organization and \a application in the beginning. This function
 *   moves those old config files to their new location if needed. This extra handling will likely removed at some point. Note that
 *   I moved away from using values from QCoreApplication to avoid having spaces and additional config suffixes in the file name.
 */
std::unique_ptr<QSettings> getSettings(const QString &organization, const QString &application)
{
    auto settings = std::unique_ptr<QSettings>();
    if (const auto portableFile
        = QFile(application.isEmpty() ? organization + QStringLiteral(".ini") : organization % QChar('/') % application % QStringLiteral(".ini"));
        portableFile.exists()) {
        settings = std::make_unique<QSettings>(portableFile.fileName(), QSettings::IniFormat);
    } else {
        settings = std::make_unique<QSettings>(QSettings::IniFormat, QSettings::UserScope, organization, application);
        // move config created by older versions to new location
        if (organization != QCoreApplication::organizationName() || application != QCoreApplication::applicationName()) {
            const auto oldConfig
                = QSettings(QSettings::IniFormat, QSettings::UserScope, QCoreApplication::organizationName(), QCoreApplication::applicationName())
                      .fileName();
            QFile::rename(oldConfig, settings->fileName()) || QFile::remove(oldConfig);
        }
    }
    settings->sync();
    return settings;
}

// namespace ApplicationInstances

} // namespace QtUtilities
