#include "qtconfigarguments.h"

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

namespace ApplicationUtilities {

QtConfigArguments::QtConfigArguments() :
    m_qtWidgetsGuiArg("qt-widgets-gui", "g", "shows a Qt widgets based graphical user interface"),
    m_qtQuickGuiArg("qt-quick-gui", "q", "shows a Qt quick based graphical user interface"),
    m_lngArg("lang", "l", "sets the language for the Qt GUI"),
    m_qmlDebuggerArg("qmljsdebugger", "qmljsdebugger", "enables QML debugging (see http://doc.qt.io/qt-5/qtquick-debugging.html)"),
    m_styleArg("style", string(), "sets the Qt widgets style"),
    m_iconThemeArg("icon-theme", string(), "sets the icon theme for the Qt GUI"),
    m_fontArg("font", string(), "sets the font family and size (point) for the Qt GUI")
{
    // language
    m_lngArg.setValueNames({"language"});
    m_lngArg.setRequiredValueCount(1);
    m_lngArg.setRequired(false);
    m_lngArg.setCallback([] (const StringVector &values) {
        QLocale::setDefault(QLocale(QString::fromLocal8Bit(values.front().c_str())));
    });
    // qml debugger (handled by Qt, just to let the parser know of it)
    m_qmlDebuggerArg.setValueNames({"port:<port_from>[,port_to][,host:<ip address>][,block]"});
    m_qmlDebuggerArg.setRequiredValueCount(1);
    // appearance
    m_styleArg.setValueNames({"style name"});
    m_styleArg.setRequiredValueCount(1);
    m_styleArg.setCallback([] (const StringVector &values) {
#ifdef GUI_QTWIDGETS
        if(QStyle *style = QStyleFactory::create(QString::fromLocal8Bit(values.front().c_str()))) {
            QApplication::setStyle(style);
        } else {
            cout << "Warning: Can not find the specified style." << endl;
        }
#endif
#ifdef GUI_QTQUICK
        Q_UNUSED(values)
        cout << "Warning: Can not set a style for the Qt Quick GUI." << endl;
#endif
    });
    m_iconThemeArg.setValueNames({"theme name"});
    m_iconThemeArg.setRequiredValueCount(1);
    m_iconThemeArg.setCallback([] (const StringVector &values) {
       QIcon::setThemeName(QString::fromLocal8Bit(values.front().c_str()));
    });
    m_fontArg.setValueNames({"name", "size"});
    m_fontArg.setRequiredValueCount(2);
    m_fontArg.setDefault(true);
#ifdef Q_OS_WIN32
    m_fontArg.setDefaultValues({"Segoe UI", "9"});
#else
    m_fontArg.setDefaultValues({string(), string()});
#endif
    m_fontArg.setCallback([] (const StringVector &values) {
        if(!values.front().empty()) {
            QFont font;
            font.setFamily(QString::fromLocal8Bit(values.front().c_str()));
            bool ok;
            int size = QString::fromLocal8Bit(values.back().c_str()).toInt(&ok);
            if(ok) {
                font.setPointSize(size);
            } else {
                cout << "Warning: Can not parse specified font size. It will be ignored." << endl;
            }
            QGuiApplication::setFont(font);
        }
    });
    m_qtWidgetsGuiArg.setSecondaryArguments({&m_lngArg, &m_qmlDebuggerArg, &m_styleArg, &m_iconThemeArg, &m_fontArg});
    m_qtQuickGuiArg.setSecondaryArguments({&m_lngArg, &m_qmlDebuggerArg, &m_iconThemeArg, &m_fontArg});
#if defined GUI_QTWIDGETS
    m_qtWidgetsGuiArg.setDefault(true);
#elif defined GUI_QTQUICK
    m_qtQuickGuiArg.setDefault(true);
#endif
}

}
