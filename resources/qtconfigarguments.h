#ifndef QTCONFIGARGUMENTS_H
#define QTCONFIGARGUMENTS_H

#include <c++utilities/application/argumentparser.h>

namespace ApplicationUtilities {

class LIB_EXPORT QtConfigArguments
{
public:
    QtConfigArguments();

    Argument &qtWidgetsGuiArg();
    Argument &qtQuickGuiArg();
    Argument &languageArg();

    bool areQtGuiArgsPresent() const;

private:
    Argument m_qtWidgetsGuiArg;
    Argument m_qtQuickGuiArg;
    Argument m_lngArg;
    Argument m_qmlDebuggerArg;
    Argument m_styleArg;
    Argument m_iconThemeArg;
    Argument m_fontArg;
};

inline Argument &QtConfigArguments::qtWidgetsGuiArg()
{
    return m_qtWidgetsGuiArg;
}

inline Argument &QtConfigArguments::qtQuickGuiArg()
{
    return m_qtQuickGuiArg;
}

inline Argument &QtConfigArguments::languageArg()
{
    return m_lngArg;
}

inline bool QtConfigArguments::areQtGuiArgsPresent() const
{
    return m_qtWidgetsGuiArg.isPresent() || m_qtQuickGuiArg.isPresent();
}

}

#endif // QTCONFIGARGUMENTS_H

#ifdef QT_CONFIG_ARGUMENTS
#undef QT_CONFIG_ARGUMENTS
#endif
#define QT_CONFIG_ARGUMENTS ApplicationUtilities::QtConfigArguments

