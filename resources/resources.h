#ifndef APPLICATION_UTILITIES_RESOURCES_H
#define APPLICATION_UTILITIES_RESOURCES_H

#include <c++utilities/application/global.h>

#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QStringList)
QT_FORWARD_DECLARE_CLASS(QSettings)

#define SET_QT_APPLICATION_INFO \
    QCoreApplication::setOrganizationName(QStringLiteral(APP_AUTHOR)); \
    QCoreApplication::setOrganizationDomain(QStringLiteral(APP_URL)); \
    QCoreApplication::setApplicationName(QStringLiteral(APP_NAME)); \
    QCoreApplication::setApplicationVersion(QStringLiteral(APP_VERSION))

#define LOAD_QT_TRANSLATIONS \
    TranslationFiles::loadQtTranslationFile(); \
    TranslationFiles::loadApplicationTranslationFile(QStringLiteral(PROJECT_NAME))

namespace QtUtilitiesResources {

LIB_EXPORT void init();
LIB_EXPORT void cleanup();

}

namespace TranslationFiles {

LIB_EXPORT void loadQtTranslationFile();
LIB_EXPORT void loadApplicationTranslationFile(const QString &applicationName);
LIB_EXPORT void loadApplicationTranslationFile(const QString &applicationName, const QString &localeName);

}

namespace ApplicationInstances {

#if defined(GUI_QTWIDGETS)
LIB_EXPORT bool hasWidgetsApp();
#endif
#if defined(GUI_QTWIDGETS) || defined(GUI_QTQUICK)
LIB_EXPORT bool hasGuiApp();
#endif
LIB_EXPORT bool hasCoreApp();

}

namespace ConfigFile {

LIB_EXPORT QString locateConfigFile(const QString &applicationName, const QString &fileName, const QSettings *settings = nullptr);

}

#endif // APPLICATION_UTILITIES_RESOURCES_H
