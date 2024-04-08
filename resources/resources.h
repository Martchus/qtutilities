#ifndef APPLICATION_UTILITIES_RESOURCES_H
#define APPLICATION_UTILITIES_RESOURCES_H

#include "../global.h"

#include <QString>
#include <QtContainerFwd>
#include <QtGlobal>

#include <initializer_list>
#include <memory>

QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QSettings)
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
QT_FORWARD_DECLARE_CLASS(QStringList)
#endif

/*!
 * \brief Sets the base name of the desktop entry for this application from buildsystem-provided meta-data.
 * \remarks
 * - This is done as part of SET_QT_APPLICATION_INFO and thus normally doesn't need to be invoked individually.
 * - This macro is still experimental.
 */
#define SET_QT_DESKTOP_FILE_NAME
#if defined(Q_OS_LINUX) && defined(qGuiApp) && defined(APP_ID)
#undef SET_QT_DESKTOP_FILE_NAME
#define SET_QT_DESKTOP_FILE_NAME QGuiApplication::setDesktopFileName(QStringLiteral(APP_ID));
#endif

/*!
 * \brief Sets the application meta data in the QCoreApplication singleton and attributes commonly used
 *        within my applications.
 * \sa ::QtUtilities::setupCommonQtApplicationAttributes()
 */
#define SET_QT_APPLICATION_INFO                                                                                                                      \
    QCoreApplication::setOrganizationName(QStringLiteral(APP_AUTHOR));                                                                               \
    QCoreApplication::setOrganizationDomain(QStringLiteral(APP_DOMAIN));                                                                             \
    QCoreApplication::setApplicationName(QStringLiteral(APP_NAME));                                                                                  \
    QCoreApplication::setApplicationVersion(QStringLiteral(APP_VERSION));                                                                            \
    SET_QT_DESKTOP_FILE_NAME                                                                                                                         \
    ::QtUtilities::setupCommonQtApplicationAttributes()

/*!
 * \brief Loads translations for Qt, other dependencies and the application.
 */
#define LOAD_QT_TRANSLATIONS                                                                                                                         \
    QtUtilities::TranslationFiles::loadQtTranslationFile(QT_TRANSLATION_FILES);                                                                      \
    QtUtilities::TranslationFiles::loadApplicationTranslationFile(QStringLiteral(PROJECT_CONFIG_NAME), APP_SPECIFIC_QT_TRANSLATION_FILES)

namespace QtUtilities {

namespace QtUtilitiesResources {

QT_UTILITIES_EXPORT void init();
QT_UTILITIES_EXPORT void cleanup();
} // namespace QtUtilitiesResources

namespace TranslationFiles {

QT_UTILITIES_EXPORT QString &additionalTranslationFilePath();
QT_UTILITIES_EXPORT void loadQtTranslationFile(std::initializer_list<QString> repositoryNames);
QT_UTILITIES_EXPORT void loadQtTranslationFile(std::initializer_list<QString> repositoryNames, const QString &localeName);
QT_UTILITIES_EXPORT void loadApplicationTranslationFile(const QString &configName, const QString &applicationName);
QT_UTILITIES_EXPORT void loadApplicationTranslationFile(const QString &configName, const QString &applicationName, const QString &localeName);
QT_UTILITIES_EXPORT void loadApplicationTranslationFile(const QString &configName, const std::initializer_list<QString> &applicationNames);
QT_UTILITIES_EXPORT void loadApplicationTranslationFile(
    const QString &configName, const std::initializer_list<QString> &applicationNames, const QString &localeName);
QT_UTILITIES_EXPORT void clearTranslationFiles();
} // namespace TranslationFiles

namespace ApplicationInstances {

#if defined(QT_UTILITIES_GUI_QTWIDGETS)
QT_UTILITIES_EXPORT bool hasWidgetsApp();
#endif
#if defined(QT_UTILITIES_GUI_QTWIDGETS) || defined(QT_UTILITIES_GUI_QTQUICK)
QT_UTILITIES_EXPORT bool hasGuiApp();
#endif
QT_UTILITIES_EXPORT bool hasCoreApp();
} // namespace ApplicationInstances

QT_UTILITIES_EXPORT void setupCommonQtApplicationAttributes();
QT_UTILITIES_EXPORT std::unique_ptr<QSettings> getSettings(const QString &organization, const QString &application = QString());
QT_UTILITIES_EXPORT QString errorMessageForSettings(const QSettings &settings);

} // namespace QtUtilities

#endif // APPLICATION_UTILITIES_RESOURCES_H
