#ifndef APPLICATION_UTILITIES_QTCONFIGARGUMENTS_H
#define APPLICATION_UTILITIES_QTCONFIGARGUMENTS_H

#include "../global.h"

#include <c++utilities/application/argumentparser.h>

#ifdef QT_UTILITIES_GUI_QTQUICK
#include <QQuickStyle>

#if defined(PLATFORM_ANDROID)
#define QT_UTILITIES_DEFAULT_QQC2_STYLE "material"
#elif defined(PLATFORM_WINDOWS)
#define QT_UTILITIES_DEFAULT_QQC2_STYLE "universal"
#endif
#endif

namespace ApplicationUtilities {

class QT_UTILITIES_EXPORT QtConfigArguments {
public:
    QtConfigArguments();

    Argument &qtWidgetsGuiArg();
    Argument &qtQuickGuiArg();
    Argument &languageArg();

    bool areQtGuiArgsPresent() const;
    void applySettings(bool preventApplyingDefaultFont = false) const;
#ifdef QT_UTILITIES_GUI_QTQUICK
    void applySettingsForQuickGui() const;
#endif

private:
    Argument m_qtWidgetsGuiArg;
    Argument m_qtQuickGuiArg;
    Argument m_lngArg;
    Argument m_qmlDebuggerArg;
    Argument m_styleArg;
    Argument m_iconThemeArg;
    Argument m_fontArg;
    Argument m_libraryPathsArg;
    Argument m_platformThemeArg;
};

/*!
 * \brief Returns the argument for the Qt Widgets GUI.
 */
inline Argument &QtConfigArguments::qtWidgetsGuiArg()
{
    return m_qtWidgetsGuiArg;
}

/*!
 * \brief Returns the argument for the Qt Quick GUI.
 */
inline Argument &QtConfigArguments::qtQuickGuiArg()
{
    return m_qtQuickGuiArg;
}

/*!
 * \brief Returns the language argument.
 */
inline Argument &QtConfigArguments::languageArg()
{
    return m_lngArg;
}

/*!
 * \brief Returns whether at least one of the GUI arguments is present.
 */
inline bool QtConfigArguments::areQtGuiArgsPresent() const
{
    return m_qtWidgetsGuiArg.isPresent() || m_qtQuickGuiArg.isPresent();
}

#ifdef QT_UTILITIES_GUI_QTQUICK
/*!
 * \brief Applies settings the for Qt Quick GUI.
 */
inline void QtConfigArguments::applySettingsForQuickGui() const
{
    if (!m_qtQuickGuiArg.isPresent()) {
        return;
    }
    if (m_styleArg.isPresent()) {
        QQuickStyle::setStyle(QString::fromLocal8Bit(m_styleArg.values().front()));
    }
#ifdef QT_UTILITIES_DEFAULT_QQC2_STYLE
    else if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QuickStyle::setStyle(QStringLiteral(QT_UTILITIES_DEFAULT_QCC2_STYLE));
    }
#endif
}
#endif

} // namespace ApplicationUtilities

#endif // APPLICATION_UTILITIES_QTCONFIGARGUMENTS_H

#ifdef QT_CONFIG_ARGUMENTS
#undef QT_CONFIG_ARGUMENTS
#endif
#define QT_CONFIG_ARGUMENTS ApplicationUtilities::QtConfigArguments
