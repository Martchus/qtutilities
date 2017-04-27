#include "./resources.h"

#include "resources/config.h"

#include <QString>
#include <QLocale>
#include <QTranslator>
#include <QLibraryInfo>
#include <QFile>
#include <QDir>
#include <QStringBuilder>
#include <QSettings>
#if defined(QT_UTILITIES_GUI_QTWIDGETS)
# include <QApplication>
# include <QIcon>
# include <QFont>
# include <QStyleFactory>
#elif defined(QT_UTILITIES_GUI_QTQUICK)
# include <QGuiApplication>
# include <QIcon>
# include <QFont>
#else
# include <QCoreApplication>
#endif

#include <iostream>

using namespace std;

///! \cond
inline void initResources() {
    Q_INIT_RESOURCE(qtutilsicons);
}

inline void cleanupResources() {
    Q_CLEANUP_RESOURCE(qtutilsicons);
}
///! \endcond

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

}

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
 * \brief Loads and installs the appropriate Qt translation file for the current locale.
 * \param repositoryNames Specifies the names of the Qt repositories to load translations for (eg. qtbase, qtscript, ...).
 * \remarks
 *  - Translation files have to be placed in one of the following locations:
 *    * QLibraryInfo::location(QLibraryInfo::TranslationsPath) (used in UNIX)
 *    * ../share/qt/translations (used in Windows)
 *  - Translation files can also be built-in using by setting the CMake variable BUILTIN_TRANSLATIONS.
 *    In this case it is also necessary to load the translations using this function.
 */
void loadQtTranslationFile(std::initializer_list<QString> repositoryNames)
{
    loadQtTranslationFile(repositoryNames, QLocale().name());
}

/*!
 * \brief Loads and installs the appropriate Qt translation file for the specified locale.
 * \param repositoryNames Specifies the names of the Qt repositories to load translations for (eg. qtbase, qtscript, ...).
 * \param localeName Specifies the name of the locale.
 * \remarks
 *  - Translation files have to be placed in one of the following locations:
 *    * QLibraryInfo::location(QLibraryInfo::TranslationsPath) (used in UNIX)
 *    * ../share/qt/translations (used in Windows)
 *  - Translation files can also be built-in using by setting the CMake variable BUILTIN_TRANSLATIONS.
 *    In this case it is also necessary to load the translations using this function.
 */
void loadQtTranslationFile(initializer_list<QString> repositoryNames, const QString &localeName)
{
    for(const QString &repoName : repositoryNames) {
        QTranslator *qtTranslator = new QTranslator;
        const QString fileName(repoName % QChar('_') % localeName);
        if(!additionalTranslationFilePath().isEmpty() && qtTranslator->load(fileName, additionalTranslationFilePath())) {
            QCoreApplication::installTranslator(qtTranslator);
        } else if(qtTranslator->load(fileName, QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
            QCoreApplication::installTranslator(qtTranslator);
        } else if(qtTranslator->load(fileName, QStringLiteral("../share/qt/translations"))) {
            // used in Windows
            QCoreApplication::installTranslator(qtTranslator);
        } else if(qtTranslator->load(fileName, QStringLiteral(":/translations"))) {
            QCoreApplication::installTranslator(qtTranslator);
        } else {
            delete qtTranslator;
            cerr << "Unable to load translation file for Qt repository \"" << repoName.toLocal8Bit().data() << "\" and language " << localeName.toLocal8Bit().data() << "." << endl;
        }
    }
}

/*!
 * \brief Loads and installs the appropriate application translation file for the current locale.
 * \param applicationName Specifies the name of the application.
 * \remarks
 *  - Translation files have to be placed in one of the following locations:
 *    * ./translations
 *    * /usr/share/$application/translations (used in UNIX)
 *    * ../share/$application/translations (used in Windows)
 *  - Translation files must be named using the following scheme:
 *    * $application_$language.qm
 *  - Translation files can also be built-in using by setting the CMake variable BUILTIN_TRANSLATIONS.
 *    In this case it is also necessary to load the translations using this function.
 */
void loadApplicationTranslationFile(const QString &applicationName)
{
    // load English translation files as fallback
    loadApplicationTranslationFile(applicationName, QStringLiteral("en_US"));
    // load translation files for current locale
    if(QLocale().name() != QLatin1String("en_US")) {
        loadApplicationTranslationFile(applicationName, QLocale().name());
    }
}

/*!
 * \brief Loads and installs the appropriate application translation file for the specified locale.
 * \param applicationName Specifies the name of the application.
 * \param localeName Specifies the name of the locale.
 * \remarks
 *  - Translation files have to be placed in one of the following locations:
 *    * ./translations
 *    * /usr/share/$application/translations (used in UNIX)
 *    * ../share/$application/translations (used in Windows)
 *  - Translation files must be named using the following scheme:
 *    * $application_$language.qm
 *  - Translation files can also be built-in using by setting the CMake variable BUILTIN_TRANSLATIONS.
 *    In this case it is also necessary to load the translations using this function.
 */
void loadApplicationTranslationFile(const QString &applicationName, const QString &localeName)
{
    QTranslator *appTranslator = new QTranslator;
    const QString fileName(QStringLiteral("%1_%2").arg(applicationName, localeName));
    if(!additionalTranslationFilePath().isEmpty() && appTranslator->load(fileName, additionalTranslationFilePath())) {
        QCoreApplication::installTranslator(appTranslator);
    } else if(appTranslator->load(fileName, QStringLiteral("."))) {
        QCoreApplication::installTranslator(appTranslator);
    } else if(appTranslator->load(fileName, QStringLiteral("./translations"))) {
        QCoreApplication::installTranslator(appTranslator);
    } else if(appTranslator->load(fileName, QStringLiteral(APP_INSTALL_PREFIX "/share/%1/translations").arg(applicationName))) {
        QCoreApplication::installTranslator(appTranslator);
    } else if(appTranslator->load(fileName, QStringLiteral("../share/%1/translations").arg(applicationName))) {
        QCoreApplication::installTranslator(appTranslator);
    } else if(appTranslator->load(fileName, QStringLiteral(":/translations"))) {
        QCoreApplication::installTranslator(appTranslator);
    } else {
        delete appTranslator;
        cerr << "Unable to load translation file for \"" << applicationName.toLocal8Bit().data() << "\" and the language \"" << localeName.toLocal8Bit().data() << "\"." << endl;
    }
}

/*!
 * \brief Loads and installs the appropriate application translation file for the current locale.
 * \param applicationNames Specifies the names of the applications.
 */
void loadApplicationTranslationFile(const std::initializer_list<QString> &applicationNames)
{
    for(const QString &applicationName : applicationNames) {
        loadApplicationTranslationFile(applicationName);
    }
}

/*!
 * \brief Loads and installs the appropriate application translation file for the specified locale.
 * \param applicationNames Specifies the names of the applications.
 * \param localeName Specifies the name of the locale.
 */
void loadApplicationTranslationFile(const std::initializer_list<QString> &applicationNames, const QString &localeName)
{
    for(const QString &applicationName : applicationNames) {
        loadApplicationTranslationFile(applicationName, localeName);
    }
}

}

/*!
 * \brief Convenience functions to check whether a QCoreApplication/QGuiApplication/QApplication singleton has been instantiated yet.
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

}

/*!
 * \brief Provides convenience functions for handling config files.
 */
namespace ConfigFile {

/*!
 * \brief Locates the config file with the specified \a fileName for the application with the specified \a applicationName.
 * \remarks If \a settings is not nullptr, the path provided by that object is also considered.
 */
QString locateConfigFile(const QString &applicationName, const QString &fileName, const QSettings *settings)
{
    // check whether the file is in the current working directory
    if(QFile::exists(fileName)) {
        return fileName;
    } else {
        // check whether the file is in the settings directory used by QSettings
        QString path;
        if(settings) {
            path = QFileInfo(settings->fileName()).absoluteDir().absoluteFilePath(fileName);
            if(QFile::exists(path)) {
                return path;
            }
        }
        // check whether there is a user created version of the file under /etc/app/
#ifdef Q_OS_WIN32
        // use relative paths on Windows
        path = QStringLiteral("../etc/") % applicationName % QChar('/') % fileName;
        if(QFile::exists(path)) {
            return path;
        } else {
            // check whether there is the default version of the file under /usr/share/app/
            path = QStringLiteral("../share/") % applicationName % QChar('/') % fileName;
            if(QFile::exists(path)) {
                return path;
            } else {
                return QString(); // file is not present
            }
        }
#else
        path = QStringLiteral("/etc/") % applicationName % QChar('/') % fileName;
        if(QFile::exists(path)) {
            return path;
        } else {
            // check whether there is the default version of the file under /usr/share/app/
            path = QStringLiteral("/usr/share/") % applicationName % QChar('/') % fileName;
            if(QFile::exists(path)) {
                return path;
            } else {
                return QString(); // file is not present
            }
        }
#endif
    }
}

}
