#ifndef DIALOGS_QT_UTILITIES_QTSETTINGS_H
#define DIALOGS_QT_UTILITIES_QTSETTINGS_H

#include "../global.h"

#if defined(QT_UTILITIES_GUI_QTWIDGETS)
#include "./optionpage.h"
#endif

#include <QtGlobal>

#include <memory>

QT_FORWARD_DECLARE_CLASS(QFontDialog)
QT_FORWARD_DECLARE_CLASS(QSettings)

namespace QtUtilities {

class OptionCategory;
struct QtSettingsData;

#if defined(QT_UTILITIES_GUI_QTWIDGETS)
BEGIN_DECLARE_UI_FILE_BASED_OPTION_PAGE_CUSTOM_CTOR(QtAppearanceOptionPage)
public:
explicit QtAppearanceOptionPage(QtSettingsData &settings, QWidget *parentWidget = nullptr);

private:
DECLARE_SETUP_WIDGETS
QtSettingsData & m_settings;
QFontDialog *m_fontDialog;
END_DECLARE_OPTION_PAGE

BEGIN_DECLARE_UI_FILE_BASED_OPTION_PAGE_CUSTOM_CTOR(QtLanguageOptionPage)
public:
explicit QtLanguageOptionPage(QtSettingsData &settings, QWidget *parentWidget = nullptr);

private:
DECLARE_SETUP_WIDGETS
QtSettingsData & m_settings;
END_DECLARE_OPTION_PAGE

BEGIN_DECLARE_UI_FILE_BASED_OPTION_PAGE_CUSTOM_CTOR(QtEnvOptionPage)
public:
explicit QtEnvOptionPage(QtSettingsData &settings, QWidget *parentWidget = nullptr);

private:
DECLARE_SETUP_WIDGETS
QtSettingsData & m_settings;
END_DECLARE_OPTION_PAGE
#endif

class QT_UTILITIES_EXPORT QtSettings {
public:
    QtSettings();
    ~QtSettings();

#if defined(QT_UTILITIES_GUI_QTWIDGETS)
    void disableNotices();
    void setRetranslatable(bool retranslatable);
#endif
    void restore(QSettings &settings);
    void save(QSettings &settings) const;
    void apply();
    void reapplyDefaultIconTheme(bool isPaletteDark);
    void reevaluatePaletteAndDefaultIconTheme();
    bool isPaletteDark();
    bool hasCustomFont() const;
    bool hasLocaleChanged() const;
    operator QtSettingsData &() const;

    OptionCategory *category();

private:
    std::unique_ptr<QtSettingsData> m_d;
};
} // namespace QtUtilities

#if defined(QT_UTILITIES_GUI_QTWIDGETS)
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE(QtAppearanceOptionPage)
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE(QtLanguageOptionPage)
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE(QtEnvOptionPage)
#endif

#endif // DIALOGS_QT_UTILITIES_QTSETTINGS_H
