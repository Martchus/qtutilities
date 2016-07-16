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
#if defined(GUI_QTWIDGETS)
# include <QApplication>
# include <QIcon>
# include <QFont>
# include <QStyleFactory>
#elif defined(GUI_QTQUICK)
# include <QGuiApplication>
# include <QIcon>
# include <QFont>
#else
# include <QCoreApplication>
#endif

#include <iostream>

using namespace std;

/*!
 * \cond
 */
void qInitResources_qtutilsicons();
void qCleanupResources_qtutilsicons();
/*!
 * \endcond
 */

/*!
 * \brief Functions for using the resources provided by this library.
 */
namespace QtUtilitiesResources {

/*!
 * \brief Initiates the resources used and provided by this library.
 */
void init()
{
    qInitResources_qtutilsicons();
}

/*!
 * \brief Frees the resources used and provided by this library.
 */
void cleanup()
{
    qCleanupResources_qtutilsicons();
}

}

/*!
 * \brief Convenience functions to load translations for Qt and the application.
 */
namespace TranslationFiles {

/*!
 * \brief Loads and installs the appropriate Qt translation file for the current locale.
 * \remarks
 *  - Translation files have to be placed in one of the following locations:
 *    * QLibraryInfo::location(QLibraryInfo::TranslationsPath) (used in UNIX)
 *    * ../share/qt/translations (used in Windows)
 *  - Translation files can also be built-in using by setting the CMake variable BUILTIN_TRANSLATIONS.
 */
void loadQtTranslationFile()
{
    // load translation files for current locale
    loadQtTranslationFile(QLocale().name());
}

/*!
 * \brief Loads and installs the appropriate Qt translation file for the specified locale.
 * \param localeName Specifies the name of the locale.
 * \remarks
 *  - Translation files have to be placed in one of the following locations:
 *    * QLibraryInfo::location(QLibraryInfo::TranslationsPath) (used in UNIX)
 *    * ../share/qt/translations (used in Windows)
 *  - Translation files can also be built-in using by setting the CMake variable BUILTIN_TRANSLATIONS.
 */
void loadQtTranslationFile(const QString &localeName)
{
    QTranslator *qtTranslator = new QTranslator;
    const QString fileName(QStringLiteral("qtbase_%1").arg(localeName));
    if(qtTranslator->load(fileName,
                          QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
        QCoreApplication::installTranslator(qtTranslator);
    } else if(qtTranslator->load(fileName, QStringLiteral("../share/qt/translations"))) {
        // used in Windows
        QCoreApplication::installTranslator(qtTranslator);
    } else if(qtTranslator->load(fileName, QStringLiteral(":/translations"))) {
        QCoreApplication::installTranslator(qtTranslator);
    } else {
        delete qtTranslator;
        cerr << "Unable to load Qt translation file for the language " << localeName.toLocal8Bit().data() << "." << endl;
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
 */
void loadApplicationTranslationFile(const QString &applicationName)
{
    // load English translation files as fallback
    loadApplicationTranslationFile(applicationName, QStringLiteral("en_US"));
    // load translation files for current locale
    loadApplicationTranslationFile(applicationName, QLocale().name());
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
 */
void loadApplicationTranslationFile(const QString &applicationName, const QString &localeName)
{
    QTranslator *appTranslator = new QTranslator;
    const QString fileName(QStringLiteral("%1_%2").arg(applicationName, localeName));
    if(appTranslator->load(fileName, QStringLiteral("."))) {
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
        cerr << "Unable to load application translation file for the language \"" << localeName.toLocal8Bit().data() << "\"." << endl;
    }
}

}

/*!
 * \brief Convenience functions to check whether a QCoreApplication/QGuiApplication/QApplication singleton has been instantiated yet.
 */
namespace ApplicationInstances {

#if defined(GUI_QTWIDGETS)
/*!
 * \brief Returns whether a QApplication has been instantiated yet.
 */
bool hasWidgetsApp()
{
    return qobject_cast<QApplication *>(QCoreApplication::instance()) != nullptr;
}
#endif

#if defined(GUI_QTWIDGETS) || defined(GUI_QTQUICK)
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
