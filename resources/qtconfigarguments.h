#ifndef APPLICATION_UTILITIES_QTCONFIGARGUMENTS_H
#define APPLICATION_UTILITIES_QTCONFIGARGUMENTS_H

#include "../global.h"

#include <c++utilities/application/argumentparser.h>

namespace ApplicationUtilities {

class QT_UTILITIES_EXPORT QtConfigArguments {
public:
    QtConfigArguments();

    Argument &qtWidgetsGuiArg();
    Argument &qtQuickGuiArg();
    Argument &languageArg();

    bool areQtGuiArgsPresent() const;
    void applySettings(bool preventApplyingDefaultFont = false) const;

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
} // namespace ApplicationUtilities

#endif // APPLICATION_UTILITIES_QTCONFIGARGUMENTS_H

#ifdef QT_CONFIG_ARGUMENTS
#undef QT_CONFIG_ARGUMENTS
#endif
#define QT_CONFIG_ARGUMENTS ApplicationUtilities::QtConfigArguments
