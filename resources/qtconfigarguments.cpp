#include "./qtconfigarguments.h"

#include <c++utilities/conversion/stringconversion.h>

#include <QString>
#include <QLocale>
#include <QFont>
#include <QIcon>
#ifdef GUI_QTWIDGETS
# include <QApplication>
# include <QStyleFactory>
#else
# include <QGuiApplication>
#endif

#include <initializer_list>
#include <iostream>

using namespace std;
using namespace ConversionUtilities;

namespace ApplicationUtilities {

/*!
 * \brief Constructs new Qt config arguments.
 */
QtConfigArguments::QtConfigArguments() :
    m_qtWidgetsGuiArg("qt-widgets-gui", "g", "shows a Qt widgets based graphical user interface"),
    m_qtQuickGuiArg("qt-quick-gui", "q", "shows a Qt quick based graphical user interface"),
    m_lngArg("lang", "l", "sets the language for the Qt GUI"),
    m_qmlDebuggerArg("qmljsdebugger", "qmljsdebugger", "enables QML debugging (see http://doc.qt.io/qt-5/qtquick-debugging.html)"),
    m_styleArg("style", string(), "sets the Qt widgets style"),
    m_iconThemeArg("icon-theme", string(), "sets the icon theme and additional theme search paths for the Qt GUI"),
    m_fontArg("font", string(), "sets the font family and size (point) for the Qt GUI"),
    m_libraryPathsArg("library-paths", string(), "sets the list of directories to search when loading libraries (all existing paths will be deleted)")
{
    // language
    m_lngArg.setValueNames({"language"});
    m_lngArg.setRequiredValueCount(1);
    m_lngArg.setRequired(false);
    m_lngArg.setCombinable(true);
    // qml debugger (handled by Qt, just to let the parser know of it)
    m_qmlDebuggerArg.setValueNames({"port:<port_from>[,port_to][,host:<ip address>][,block]"});
    m_qmlDebuggerArg.setRequiredValueCount(1);
    m_qmlDebuggerArg.setCombinable(true);
    // appearance
    m_styleArg.setValueNames({"style name"});
    m_styleArg.setRequiredValueCount(1);
    m_styleArg.setCombinable(true);
    m_iconThemeArg.setValueNames({"theme name", "search path 1", "search path 2"});
    m_iconThemeArg.setRequiredValueCount(-1);
    m_iconThemeArg.setCombinable(true);
    m_fontArg.setValueNames({"name", "size"});
    m_fontArg.setRequiredValueCount(2);
    m_fontArg.setCombinable(true);
    m_libraryPathsArg.setValueNames({"path 1", "path 2"});
    m_libraryPathsArg.setRequiredValueCount(-1);
    m_libraryPathsArg.setCombinable(true);
    m_qtWidgetsGuiArg.setSecondaryArguments({&m_lngArg, &m_qmlDebuggerArg, &m_styleArg, &m_iconThemeArg, &m_fontArg, &m_libraryPathsArg});
    m_qtQuickGuiArg.setSecondaryArguments({&m_lngArg, &m_qmlDebuggerArg, &m_iconThemeArg, &m_fontArg, &m_libraryPathsArg});
    m_qtWidgetsGuiArg.setDenotesOperation(true);
    m_qtQuickGuiArg.setDenotesOperation(true);
#if defined GUI_QTWIDGETS
    m_qtWidgetsGuiArg.setDefault(true);
#elif defined GUI_QTQUICK
    m_qtQuickGuiArg.setDefault(true);
#endif
}

/*!
 * \brief Applies the settings from the arguments.
 * \remarks Also checks environment variables for the icon theme.
 */
void QtConfigArguments::applySettings() const
{
    if(m_lngArg.isPresent()) {
        QLocale::setDefault(QLocale(QString::fromLocal8Bit(m_lngArg.values().front().data())));
    }
    if(m_styleArg.isPresent()) {
#ifdef GUI_QTWIDGETS
        if(QStyle *style = QStyleFactory::create(QString::fromLocal8Bit(m_styleArg.values().front().data()))) {
            QApplication::setStyle(style);
        } else {
            cout << "Warning: Can not find the specified style." << endl;
        }
#endif
#ifdef GUI_QTQUICK
        cout << "Warning: Can not set a style for the Qt Quick GUI." << endl;
#endif
    }
#ifdef Q_OS_WIN32
    bool searchPathsPresent = false;
#endif
    if(m_iconThemeArg.isPresent()) {
        auto i = m_iconThemeArg.values().cbegin(), end = m_iconThemeArg.values().end();
        if(i != end) {
            if(++i != end) {
                QStringList searchPaths;
                searchPaths.reserve(m_iconThemeArg.values().size() - 1);
                for(; i != end; ++i) {
                    searchPaths << QString::fromLocal8Bit(i->data());
                }
                QIcon::setThemeSearchPaths(searchPaths);
#ifdef Q_OS_WIN32
                searchPathsPresent = !searchPaths.isEmpty();
#endif
            }
            QIcon::setThemeName(QString::fromLocal8Bit(i->data()));
        }
    } else {
        if(qEnvironmentVariableIsSet("ICON_THEME_SEARCH_PATH")) {
            QString path;
            path.append(qgetenv("ICON_THEME_SEARCH_PATH"));
            QIcon::setThemeSearchPaths(QStringList() << path);
#ifdef Q_OS_WIN32
            searchPathsPresent = true;
#endif
        }
        if(qEnvironmentVariableIsSet("ICON_THEME")) {
            QString themeName;
            themeName.append(qgetenv("ICON_THEME"));
            QIcon::setThemeName(themeName);
        }
    }
#ifdef Q_OS_WIN32
    // default configuration under Windows
    if(!searchPathsPresent) {
        QIcon::setThemeSearchPaths(QIcon::themeSearchPaths() << QStringLiteral("../share/icons"));
    }
    if(QIcon::themeName().isEmpty()) {
        QIcon::setThemeName(QStringLiteral("default"));
    }
#endif
    if(m_fontArg.isPresent()) {
        QFont font;
        font.setFamily(QString::fromLocal8Bit(m_fontArg.values().front().data()));
        try {
            font.setPointSize(stringToNumber<int>(m_fontArg.values().back()));
        } catch(const ConversionException &) {
            cout << "Warning: The specified font size is no number and will be ignored." << endl;
        }
        QGuiApplication::setFont(font);
    } else {
#ifdef Q_OS_WIN32
        QGuiApplication::setFont(QFont(QStringLiteral("Segoe UI"), 9));
#endif
    }
    if(m_libraryPathsArg.isPresent()) {
        QStringList libraryPaths;
        libraryPaths.reserve(m_libraryPathsArg.values().size());
        for(const auto &path : m_libraryPathsArg.values()) {
            libraryPaths << QString::fromLocal8Bit(path.data());
        }
        QCoreApplication::setLibraryPaths(libraryPaths);
    }
}

}
