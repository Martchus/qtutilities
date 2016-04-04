// The functions and classes declared in this header are experimental.
// API/ABI might change in minor release!

#ifndef QT_UTILITIES_QTSETTINGS_H
#define QT_UTILITIES_QTSETTINGS_H

#include "./optionpage.h"

#include <memory>

QT_FORWARD_DECLARE_CLASS(QFontDialog)

namespace Dialogs {

class OptionCategory;

OptionCategory LIB_EXPORT *qtOptionCategory(QObject *parent = nullptr);

BEGIN_DECLARE_UI_FILE_BASED_OPTION_PAGE(QtAppearanceOptionPage)
    DECLARE_SETUP_WIDGETS
    QFontDialog *m_fontDialog;
END_DECLARE_OPTION_PAGE

DECLARE_UI_FILE_BASED_OPTION_PAGE(QtLanguageOptionPage)

}

#endif // QT_UTILITIES_QTSETTINGS_H
