#include "resources.h"

#include <QString>
#include <QLocale>
#include <QTranslator>
#include <QLibraryInfo>
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

void qInitResources_qtutilsicons();
void qCleanupResources_qtutilsicons();

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

namespace TranslationFiles {

/*!
 * \brief Loads and installs the appropriate Qt translation file.
 */
void loadQtTranslationFile()
{
    QLocale locale;
    if(locale.language() != QLocale::English) {
        QTranslator *qtTranslator = new QTranslator;
        if(qtTranslator->load(QStringLiteral("qt_%1").arg(locale.name()),
                          QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
            QCoreApplication::installTranslator(qtTranslator);
        } else {
            cout << "Unable to load Qt translation file for the language " << locale.name().toStdString() << "." << endl;
        }
    }
}

/*!
 * \brief Loads and installs the appropriate application translation file.
 * \param applicationName Specifies the name of the application.
 * \remarks Translation files have to be placed in one of the following
 *          locations:
 *           - /usr/share/$application/translations
 *           - ./translations
 *          Translation files must be named using the following scheme:
 *           - $application_$language.qm
 */
void loadApplicationTranslationFile(const QString &applicationName)
{
    QLocale locale;
    QTranslator *appTranslator = new QTranslator;
    QString fileName = QStringLiteral("%1_%2").arg(applicationName, locale.name());
    if(appTranslator->load(fileName, QStringLiteral("./translations"))) {
        QCoreApplication::installTranslator(appTranslator);
    } else if(appTranslator->load(fileName, QStringLiteral("/usr/share/%1/translations").arg(applicationName))) {
        QCoreApplication::installTranslator(appTranslator);
    } else {
        cout << "Unable to load application translation file for the language " << locale.name().toStdString() << "." << endl;
    }
}

}

namespace Theme {

/*!
 * \brief Sets the default icon theme.
 */
#if defined(GUI_QTWIDGETS) || defined(GUI_QTQUICK)
void setup()
{
    if(QIcon::themeName().isEmpty()) {
        QIcon::setThemeName(QStringLiteral("oxygen"));
    }
}
#endif

}

namespace ApplicationInstances {

#if defined(GUI_QTWIDGETS)
bool hasWidgetsApp()
{
    return qobject_cast<QApplication*>(QCoreApplication::instance()) != nullptr;
}
#endif

#if defined(GUI_QTWIDGETS) || defined(GUI_QTQUICK)
bool hasGuiApp()
{
    return qobject_cast<QGuiApplication*>(QCoreApplication::instance()) != nullptr;
}
#endif

bool hasCoreApp()
{
    return qobject_cast<QCoreApplication*>(QCoreApplication::instance()) != nullptr;
}

}
