#ifndef RESOURCES_H
#define RESOURCES_H

#include <c++utilities/application/global.h>

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QString;
class QStringList;
QT_END_NAMESPACE

namespace QtUtilitiesResources {

LIB_EXPORT void init();
LIB_EXPORT void cleanup();

}

namespace TranslationFiles {

LIB_EXPORT void loadQtTranslationFile();
LIB_EXPORT void loadApplicationTranslationFile(const QString &applicationName);
LIB_EXPORT void loadApplicationTranslationFile(const QString &applicationName, const QString &localeName);

}

namespace Theme {

#if defined(GUI_QTWIDGETS) || defined(GUI_QTQUICK)
LIB_EXPORT void setup();
#endif

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

#endif // RESOURCES_H
