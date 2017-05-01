#ifndef APPLICATION_UTILITIES_RESOURCES_H
#define APPLICATION_UTILITIES_RESOURCES_H

#include "../global.h"

#include <QtGlobal>

#include <initializer_list>

QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QStringList)
QT_FORWARD_DECLARE_CLASS(QSettings)

/*!
 * \brief Sets the application meta data in the QCoreApplication singleton.
 */
#define SET_QT_APPLICATION_INFO                                                                                                                      \
    QCoreApplication::setOrganizationName(QStringLiteral(APP_AUTHOR));                                                                               \
    QCoreApplication::setOrganizationDomain(QStringLiteral(APP_URL));                                                                                \
    QCoreApplication::setApplicationName(QStringLiteral(APP_NAME));                                                                                  \
    QCoreApplication::setApplicationVersion(QStringLiteral(APP_VERSION));                                                                            \
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true)

/*!
 * \brief Loads translations for Qt, other dependencies and the application.
 */
#define LOAD_QT_TRANSLATIONS                                                                                                                         \
    TranslationFiles::loadQtTranslationFile(QT_TRANSLATION_FILES);                                                                                   \
    TranslationFiles::loadApplicationTranslationFile(APP_SPECIFIC_QT_TRANSLATION_FILES)

namespace QtUtilitiesResources {

QT_UTILITIES_EXPORT void init();
QT_UTILITIES_EXPORT void cleanup();
}

namespace TranslationFiles {

QT_UTILITIES_EXPORT QString &additionalTranslationFilePath();
QT_UTILITIES_EXPORT void loadQtTranslationFile(std::initializer_list<QString> repositoryNames);
QT_UTILITIES_EXPORT void loadQtTranslationFile(std::initializer_list<QString> repositoryNames, const QString &localeName);
QT_UTILITIES_EXPORT void loadApplicationTranslationFile(const QString &applicationName);
QT_UTILITIES_EXPORT void loadApplicationTranslationFile(const QString &applicationName, const QString &localeName);
QT_UTILITIES_EXPORT void loadApplicationTranslationFile(const std::initializer_list<QString> &applicationNames);
QT_UTILITIES_EXPORT void loadApplicationTranslationFile(const std::initializer_list<QString> &applicationNames, const QString &localeName);
}

namespace ApplicationInstances {

#if defined(QT_UTILITIES_GUI_QTWIDGETS)
QT_UTILITIES_EXPORT bool hasWidgetsApp();
#endif
#if defined(QT_UTILITIES_GUI_QTWIDGETS) || defined(QT_UTILITIES_GUI_QTQUICK)
QT_UTILITIES_EXPORT bool hasGuiApp();
#endif
QT_UTILITIES_EXPORT bool hasCoreApp();
}

namespace ConfigFile {

QT_UTILITIES_EXPORT QString locateConfigFile(const QString &applicationName, const QString &fileName, const QSettings *settings = nullptr);
}

#endif // APPLICATION_UTILITIES_RESOURCES_H
