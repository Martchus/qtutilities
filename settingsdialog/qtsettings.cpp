#include "./qtsettings.h"
#include "./optioncategory.h"
#include "./optioncategoryfiltermodel.h"
#include "./optioncategorymodel.h"
#include "./optionpage.h"

#include "../paletteeditor/paletteeditor.h"

#include "../widgets/clearlineedit.h"

#include "../resources/resources.h"

#include "../misc/desktoputils.h"

#include "resources/config.h"

#include "ui_qtappearanceoptionpage.h"
#include "ui_qtenvoptionpage.h"
#include "ui_qtlanguageoptionpage.h"

#include <c++utilities/application/commandlineutils.h>

#include <QDir>
#include <QFileDialog>
#include <QFontDialog>
#include <QIcon>
#include <QOperatingSystemVersion>
#include <QSettings>
#include <QStringBuilder>
#include <QStyleFactory>

#if defined(Q_OS_WINDOWS) && (QT_VERSION >= QT_VERSION_CHECK(6, 3, 0))
#include <QOperatingSystemVersion>
#define QT_UTILITIES_USE_FUSION_ON_WINDOWS_11
#endif

#include <iostream>
#include <memory>
#include <optional>

namespace QtUtilities {

struct QtSettingsData {
    QtSettingsData();

    QFont font;
    std::optional<QFont> initialFont;
    QPalette palette; // the currently applied palette (only in use if customPalette is true, though)
    QPalette selectedPalette; // the intermediately selected palette (chosen in palette editor but not yet applied)
    QString widgetStyle;
    QString initialWidgetStyle;
    QString styleSheetPath;
    QString iconTheme;
    QString initialIconTheme;
    QLocale defaultLocale;
    QLocale previousLocale;
    QString localeName;
    QString previousPluginDirectory;
    QString additionalPluginDirectory;
    QString previousIconThemeSearchPath;
    QString additionalIconThemeSearchPath;
    bool customFont;
    bool customPalette;
    bool customWidgetStyle;
    bool customStyleSheet;
    bool customIconTheme;
    bool customLocale;
    bool isPaletteDark;
    bool showNotices;
    bool retranslatable;
};

inline QtSettingsData::QtSettingsData()
    : iconTheme(QIcon::themeName())
    , initialIconTheme(iconTheme)
    , localeName(defaultLocale.name())
    , customFont(false)
    , customPalette(false)
    , customWidgetStyle(false)
    , customStyleSheet(false)
    , customIconTheme(false)
    , customLocale(false)
    , isPaletteDark(false)
    , showNotices(true)
    , retranslatable(false)
{
}

/*!
 * \brief Creates a new settings object.
 * \remarks Settings are not restored automatically. Instead, some values (font,
 * widget style, ...) are initialized
 *          from the current Qt configuration. These values are considered as
 * system-default.
 */
QtSettings::QtSettings()
    : m_d(std::make_unique<QtSettingsData>())
{
}

/*!
 * \brief Destroys the settings object.
 * \remarks Unlike QSettings not explicitly saved settings are not saved
 * automatically.
 */
QtSettings::~QtSettings()
{
}

/*!
 * \brief Disables notices on option pages that settings take only effect after restarting.
 * \remarks
 * - This function must be called before obtaining the option pages via category().
 * - Only call this function if the application actually re-applies these settings when the
 *   settings dialog is applied and when it is generally able to handle widget style and
 *   palette changes well.
 * - The localization option page's notice is handled via setRetranslatable() and the notice
 *   for the environment page is still always shown (as those settings can never be applied
 *   at runtime). So this affects only the appearance page at this point.
 */
void QtSettings::disableNotices()
{
    m_d->showNotices = false;
}

/*!
 * \brief Sets whether the application supports changing the locale settings at runtime.
 * \remarks
 * Set this to true if the application will retranslate its UI after the locale has changed.
 * This requires the application to re-install translators and to re-invoke all ts() and
 * translate() function calls. If set to true, the notice that the locale setting takes only
 * effect after restarting is not shown anymore.
 */
void QtSettings::setRetranslatable(bool retranslatable)
{
    m_d->retranslatable = retranslatable;
}

/*!
 * \brief Returns whether a custom font is set.
 */
bool QtSettings::hasCustomFont() const
{
    return m_d->customFont;
}

/*!
 * \brief Restores the settings from the specified QSettings object.
 * \remarks The restored values are not applied automatically (except
 * translation path).
 * \sa apply(), save()
 */
void QtSettings::restore(QSettings &settings)
{
    settings.beginGroup(QStringLiteral("qt"));
    m_d->font.fromString(settings.value(QStringLiteral("font")).toString());
    m_d->customFont = settings.value(QStringLiteral("customfont"), false).toBool();
    m_d->palette = settings.value(QStringLiteral("palette")).value<QPalette>();
    m_d->customPalette = settings.value(QStringLiteral("custompalette"), false).toBool();
    m_d->widgetStyle = settings.value(QStringLiteral("widgetstyle"), m_d->widgetStyle).toString();
    m_d->customWidgetStyle = settings.value(QStringLiteral("customwidgetstyle"), false).toBool();
    m_d->styleSheetPath = settings.value(QStringLiteral("stylesheetpath"), m_d->styleSheetPath).toString();
    m_d->customStyleSheet = settings.value(QStringLiteral("customstylesheet"), false).toBool();
    m_d->iconTheme = settings.value(QStringLiteral("icontheme"), m_d->iconTheme).toString();
    m_d->customIconTheme = settings.value(QStringLiteral("customicontheme"), false).toBool();
    m_d->localeName = settings.value(QStringLiteral("locale"), m_d->localeName).toString();
    m_d->customLocale = settings.value(QStringLiteral("customlocale"), false).toBool();
    m_d->additionalPluginDirectory = settings.value(QStringLiteral("plugindir")).toString();
    m_d->additionalIconThemeSearchPath = settings.value(QStringLiteral("iconthemepath")).toString();
    TranslationFiles::additionalTranslationFilePath() = settings.value(QStringLiteral("trpath")).toString();
    settings.endGroup();
}

/*!
 * \brief Saves the settings to the specified QSettings object.
 */
void QtSettings::save(QSettings &settings) const
{
    settings.beginGroup(QStringLiteral("qt"));
    settings.setValue(QStringLiteral("font"), QVariant(m_d->font.toString()));
    settings.setValue(QStringLiteral("customfont"), m_d->customFont);
    settings.setValue(QStringLiteral("palette"), QVariant(m_d->palette));
    settings.setValue(QStringLiteral("custompalette"), m_d->customPalette);
    settings.setValue(QStringLiteral("widgetstyle"), m_d->widgetStyle);
    settings.setValue(QStringLiteral("customwidgetstyle"), m_d->customWidgetStyle);
    settings.setValue(QStringLiteral("stylesheetpath"), m_d->styleSheetPath);
    settings.setValue(QStringLiteral("customstylesheet"), m_d->customStyleSheet);
    settings.setValue(QStringLiteral("icontheme"), m_d->iconTheme);
    settings.setValue(QStringLiteral("customicontheme"), m_d->customIconTheme);
    settings.setValue(QStringLiteral("locale"), m_d->localeName);
    settings.setValue(QStringLiteral("customlocale"), m_d->customLocale);
    settings.setValue(QStringLiteral("plugindir"), m_d->additionalPluginDirectory);
    settings.setValue(QStringLiteral("iconthemepath"), m_d->additionalIconThemeSearchPath);
    settings.setValue(QStringLiteral("trpath"), QVariant(TranslationFiles::additionalTranslationFilePath()));
    settings.endGroup();
}

/*!
 * \brief Returns the icon themes present in the specified \a searchPaths.
 * \remarks The display name is the key and the actual icon theme name the value.
 *          This way the map is sorted correctly for display purposes.
 */
static QMap<QString, QString> scanIconThemes(const QStringList &searchPaths)
{
    auto res = QMap<QString, QString>();
    for (const auto &searchPath : searchPaths) {
        const auto dir = QDir(searchPath).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const auto &iconTheme : dir) {
            auto indexFile = QFile(searchPath % QChar('/') % iconTheme % QStringLiteral("/index.theme"));
            auto index = QByteArray();
            if (indexFile.open(QFile::ReadOnly) && !(index = indexFile.readAll()).isEmpty()) {
                const auto iconThemeSection = index.indexOf("[Icon Theme]");
                const auto nameStart = index.indexOf("Name=", iconThemeSection != -1 ? iconThemeSection : 0);
                if (nameStart != -1) {
                    auto nameLength = index.indexOf("\n", nameStart) - nameStart - 5;
                    if (nameLength > 0) {
                        auto displayName = QString::fromUtf8(index.mid(nameStart + 5, nameLength));
                        if (displayName != iconTheme) {
                            displayName += QChar(' ') % QChar('(') % iconTheme % QChar(')');
                        }
                        res[displayName] = iconTheme;
                        continue;
                    }
                }
            }
            res[iconTheme] = iconTheme;
        }
    }
    return res;
}

/*!
 * \brief Applies the current configuration.
 * \remarks
 * QApplication/QGuiApplication must be instantiated before calling this
 * method. Hence it makes most sense to call this directly after instantiating
 * QApplication/QGuiApplication.
 *
 * This function may be called multiple times. This supports restoring default
 * settings (e.g. if use of a custom icon theme has been disabled again after it
 * was enabled the default icon theme will be configured again).
 */
void QtSettings::apply()
{
    // apply environment
    if (m_d->additionalPluginDirectory != m_d->previousPluginDirectory) {
        if (!m_d->previousPluginDirectory.isEmpty()) {
            QCoreApplication::removeLibraryPath(m_d->previousPluginDirectory);
        }
        if (!m_d->additionalPluginDirectory.isEmpty()) {
            QCoreApplication::addLibraryPath(m_d->additionalPluginDirectory);
        }
        m_d->previousPluginDirectory = m_d->additionalPluginDirectory;
    }
    if (m_d->additionalIconThemeSearchPath != m_d->previousIconThemeSearchPath) {
        auto paths = QIcon::themeSearchPaths();
        if (!m_d->previousIconThemeSearchPath.isEmpty()) {
            paths.removeAll(m_d->previousIconThemeSearchPath);
        }
        if (!m_d->additionalIconThemeSearchPath.isEmpty()) {
            paths.append(m_d->additionalIconThemeSearchPath);
        }
        m_d->previousIconThemeSearchPath = m_d->additionalIconThemeSearchPath;
        QIcon::setThemeSearchPaths(paths);
    }

    // read style sheet
    auto styleSheet = QString();
    if (m_d->customStyleSheet && !m_d->styleSheetPath.isEmpty()) {
        auto file = QFile(m_d->styleSheetPath);
        if (!file.open(QFile::ReadOnly)) {
            std::cerr << "Unable to open the specified stylesheet \"" << m_d->styleSheetPath.toLocal8Bit().data() << "\"." << std::endl;
        }
        styleSheet.append(file.readAll());
        if (file.error() != QFile::NoError) {
            std::cerr << "Unable to read the specified stylesheet \"" << m_d->styleSheetPath.toLocal8Bit().data() << "\"." << std::endl;
        }
    }

    // apply appearance
    if (m_d->customFont) {
        if (!m_d->initialFont.has_value()) {
            m_d->initialFont = QGuiApplication::font();
        }
        QGuiApplication::setFont(m_d->font);
    } else if (m_d->initialFont.has_value()) {
        QGuiApplication::setFont(m_d->initialFont.value());
    }
#ifdef QT_UTILITIES_USE_FUSION_ON_WINDOWS_11
    if (m_d->initialWidgetStyle.isEmpty()) {
        // use Fusion on Windows 11 as the native style doesn't look good
        // see https://bugreports.qt.io/browse/QTBUG-97668
        if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows11) {
            m_d->initialWidgetStyle = QStringLiteral("Fusion");
        }
    }
#endif
    if (m_d->customWidgetStyle) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 1, 0)
        const auto *const currentStyle = QApplication::style();
        if (m_d->initialWidgetStyle.isEmpty() && currentStyle) {
            m_d->initialWidgetStyle = currentStyle->name();
        }
#endif
        QApplication::setStyle(m_d->widgetStyle);
    } else if (!m_d->initialWidgetStyle.isEmpty()) {
        QApplication::setStyle(m_d->initialWidgetStyle);
    }
    if (auto *const qapp = qobject_cast<QApplication *>(QApplication::instance())) {
        qapp->setStyleSheet(styleSheet);
    } else {
        std::cerr << "Unable to apply the specified stylesheet \"" << m_d->styleSheetPath.toLocal8Bit().data()
                  << "\" because no QApplication has been instantiated." << std::endl;
    }
    if (m_d->customPalette) {
        QGuiApplication::setPalette(m_d->palette);
    } else {
        QGuiApplication::setPalette(QPalette());
    }
    m_d->isPaletteDark = isPaletteDark();
    if (m_d->customIconTheme) {
        QIcon::setThemeName(m_d->iconTheme);
    } else if (!m_d->initialIconTheme.isEmpty()) {
        if (m_d->iconTheme != m_d->initialIconTheme) {
            // set the icon theme back to what it was before changing anything (not sure how to read the current system icon theme again)
            QIcon::setThemeName(m_d->initialIconTheme);
        }
    } else {
        // use bundled default icon theme matching the current palette
        // notes: - It is ok that search paths specified via CLI arguments are not set here yet. When doing so one should also
        //          specify the desired icon theme explicitly.
        //        - The icon themes "default" and "default-dark" come from QtConfig.cmake which makes the first non-dark bundled
        //          icon theme available as "default" and the first dark icon theme available as "default-dark". An icon theme
        //          is considered dark if it ends with "-dark".
        const auto bundledIconThemes = scanIconThemes(QStringList(QStringLiteral(":/icons")));
        if (m_d->isPaletteDark && bundledIconThemes.contains(QStringLiteral("default-dark"))) {
            QIcon::setThemeName(QStringLiteral("default-dark"));
        } else if (bundledIconThemes.contains(QStringLiteral("default"))) {
            QIcon::setThemeName(QStringLiteral("default"));
        }
    }

    // apply locale
    m_d->previousLocale = QLocale();
    QLocale::setDefault(m_d->customLocale ? QLocale(m_d->localeName) : m_d->defaultLocale);

    // log some debug information on the first call if env variable set
    static auto debugInfoLogged = false;
    if (debugInfoLogged) {
        return;
    }
    const auto debugLoggingEnabled = CppUtilities::isEnvVariableSet(PROJECT_VARNAME_UPPER "_LOG_QT_CONFIG");
    if (debugLoggingEnabled.has_value() && debugLoggingEnabled.value()) {
        if (const auto os = QOperatingSystemVersion::current(); os.type() != QOperatingSystemVersion::Unknown) {
            std::cerr << "OS name and version: " << os.name().toStdString() << ' ' << os.version().toString().toStdString() << '\n';
        }
        std::cerr << "Qt version: " << qVersion() << '\n';
        std::cerr << "Qt platform (set QT_QPA_PLATFORM to override): " << QGuiApplication::platformName().toStdString() << '\n';
        std::cerr << "Qt locale: " << QLocale().name().toStdString() << '\n';
        std::cerr << "Qt library paths: " << QCoreApplication::libraryPaths().join(':').toStdString() << '\n';
        std::cerr << "Qt theme search paths: " << QIcon::themeSearchPaths().join(':').toStdString() << '\n';
        debugInfoLogged = true;
    }
}

/*!
 * \brief Re-evaluates whether the palette is dark and re-applies default icon theme.
 *
 * Re-assigns the appropriate default icon theme depending on the current palette. Call this function after the palette
 * has changed.
 *
 * \remarks
 * - The default icon theme must have been assigned before using the apply() function.
 * - This function has no effect if a custom icon theme is configured.
 */
void QtSettings::reevaluatePaletteAndDefaultIconTheme()
{
    const auto isPaletteDark = QtUtilities::isPaletteDark();
    if (isPaletteDark == m_d->isPaletteDark) {
        return; // no need to do anything if there's no change
    }
    m_d->isPaletteDark = isPaletteDark;
    if (auto iconTheme = QIcon::themeName(); iconTheme == QStringLiteral("default") || iconTheme == QStringLiteral("default-dark")) {
        QIcon::setThemeName(m_d->isPaletteDark ? QStringLiteral("default-dark") : QStringLiteral("default"));
    }
}

/*!
 * \brief Returns whether the palette is dark.
 * \remarks
 * Changes to the palette since the last call to apply() and reevaluatePaletteAndDefaultIconTheme() are not taken
 * into account.
 */
bool QtSettings::isPaletteDark()
{
    return m_d->isPaletteDark;
}

/*!
 * \brief Returns whether the last apply() call has changed the default locale.
 */
bool QtSettings::hasLocaleChanged() const
{
    return m_d->previousLocale != QLocale();
}

/*!
 * \brief Returns a new OptionCatecory containing all Qt related option pages.
 * \remarks
 * - The QtSettings instance does not keep the ownership over the returned
 *   category.
 * - The pages of the returned category require the QtSettings instance which
 *   hence must be present as long as all pages are destroyed.
 */
OptionCategory *QtSettings::category()
{
    auto *category = new OptionCategory;
    category->setDisplayName(QCoreApplication::translate("QtGui::QtOptionCategory", "Qt"));
    category->setIcon(QIcon::fromTheme(QStringLiteral("qtcreator"), QIcon(QStringLiteral(":/qtutilities/icons/hicolor/48x48/apps/qtcreator.svg"))));
    category->assignPages({ new QtAppearanceOptionPage(*m_d), new QtLanguageOptionPage(*m_d), new QtEnvOptionPage(*m_d) });
    return category;
}

QtAppearanceOptionPage::QtAppearanceOptionPage(QtSettingsData &settings, QWidget *parentWidget)
    : QtAppearanceOptionPageBase(parentWidget)
    , m_settings(settings)
    , m_fontDialog(nullptr)
{
}

QtAppearanceOptionPage::~QtAppearanceOptionPage()
{
}

bool QtAppearanceOptionPage::apply()
{
    m_settings.font = ui()->fontComboBox->currentFont();
    m_settings.customFont = !ui()->fontCheckBox->isChecked();
    m_settings.widgetStyle = ui()->widgetStyleComboBox->currentText();
    m_settings.customWidgetStyle = !ui()->widgetStyleCheckBox->isChecked();
    m_settings.styleSheetPath = ui()->styleSheetPathSelection->lineEdit()->text();
    m_settings.customStyleSheet = !ui()->styleSheetCheckBox->isChecked();
    m_settings.palette = m_settings.selectedPalette;
    m_settings.customPalette = !ui()->paletteCheckBox->isChecked();
    m_settings.iconTheme
        = ui()->iconThemeComboBox->currentIndex() != -1 ? ui()->iconThemeComboBox->currentData().toString() : ui()->iconThemeComboBox->currentText();
    m_settings.customIconTheme = !ui()->iconThemeCheckBox->isChecked();
    return true;
}

void QtAppearanceOptionPage::reset()
{
    ui()->fontComboBox->setCurrentFont(m_settings.font);
    ui()->fontCheckBox->setChecked(!m_settings.customFont);
    ui()->widgetStyleComboBox->setCurrentText(
        m_settings.widgetStyle.isEmpty() ? (QApplication::style() ? QApplication::style()->objectName() : QString()) : m_settings.widgetStyle);
    ui()->widgetStyleCheckBox->setChecked(!m_settings.customWidgetStyle);
    ui()->styleSheetPathSelection->lineEdit()->setText(m_settings.styleSheetPath);
    ui()->styleSheetCheckBox->setChecked(!m_settings.customStyleSheet);
    m_settings.selectedPalette = m_settings.palette;
    ui()->paletteCheckBox->setChecked(!m_settings.customPalette);
    int iconThemeIndex = ui()->iconThemeComboBox->findData(m_settings.iconTheme);
    if (iconThemeIndex != -1) {
        ui()->iconThemeComboBox->setCurrentIndex(iconThemeIndex);
    } else {
        ui()->iconThemeComboBox->setCurrentText(m_settings.iconTheme);
    }
    ui()->iconThemeCheckBox->setChecked(!m_settings.customIconTheme);
}

QWidget *QtAppearanceOptionPage::setupWidget()
{
    // call base implementation first, so ui() is available
    auto *widget = QtAppearanceOptionPageBase::setupWidget();
    if (!m_settings.showNotices) {
        ui()->label->hide();
    }

    // setup widget style selection
    ui()->widgetStyleComboBox->addItems(QStyleFactory::keys());

    // setup style sheet selection
    ui()->styleSheetPathSelection->provideCustomFileMode(QFileDialog::ExistingFile);

    // setup font selection
    QObject::connect(ui()->fontPushButton, &QPushButton::clicked, widget, [this] {
        if (!m_fontDialog) {
            m_fontDialog = new QFontDialog(this->widget());
            m_fontDialog->setCurrentFont(ui()->fontComboBox->font());
            QObject::connect(m_fontDialog, &QFontDialog::fontSelected, ui()->fontComboBox, &QFontComboBox::setCurrentFont);
            QObject::connect(ui()->fontComboBox, &QFontComboBox::currentFontChanged, m_fontDialog, &QFontDialog::setCurrentFont);
        }
        m_fontDialog->show();
    });

    // setup palette selection
    QObject::connect(ui()->paletteToolButton, &QToolButton::clicked, ui()->paletteToolButton,
        [this] { m_settings.selectedPalette = PaletteEditor::getPalette(this->widget(), m_settings.selectedPalette); });

    // setup icon theme selection
    const auto iconThemes = scanIconThemes(QIcon::themeSearchPaths() << QStringLiteral("/usr/share/icons/"));
    auto *iconThemeComboBox = ui()->iconThemeComboBox;
    for (auto i = iconThemes.begin(), end = iconThemes.end(); i != end; ++i) {
        const auto &displayName = i.key();
        const auto &id = i.value();
        if (const auto existingItemIndex = iconThemeComboBox->findData(id); existingItemIndex != -1) {
            iconThemeComboBox->setItemText(existingItemIndex, displayName);
        } else {
            iconThemeComboBox->addItem(displayName, id);
        }
    }

    return widget;
}

QtLanguageOptionPage::QtLanguageOptionPage(QtSettingsData &settings, QWidget *parentWidget)
    : QtLanguageOptionPageBase(parentWidget)
    , m_settings(settings)
{
}

QtLanguageOptionPage::~QtLanguageOptionPage()
{
}

bool QtLanguageOptionPage::apply()
{
    m_settings.localeName = ui()->localeComboBox->currentText();
    m_settings.customLocale = !ui()->localeCheckBox->isChecked();
    return true;
}

void QtLanguageOptionPage::reset()
{
    ui()->localeComboBox->setCurrentText(m_settings.localeName);
    ui()->localeCheckBox->setChecked(!m_settings.customLocale);
}

QWidget *QtLanguageOptionPage::setupWidget()
{
    // call base implementation first, so ui() is available
    auto *widget = QtLanguageOptionPageBase::setupWidget();
    if (m_settings.retranslatable) {
        ui()->label->hide();
    }

    // add all available locales to combo box
    auto *localeComboBox = ui()->localeComboBox;
    const auto locales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry);
    for (const QLocale &locale : locales) {
        localeComboBox->addItem(locale.name());
    }

    auto *languageLabel = ui()->languageLabel;
    QObject::connect(ui()->localeComboBox, &QComboBox::currentTextChanged, languageLabel, [languageLabel, localeComboBox] {
        const auto selectedLocale = QLocale(localeComboBox->currentText());
        const auto currentLocale = QLocale();
        const auto territory =
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
            currentLocale.territoryToString(selectedLocale.territory());
#else
            currentLocale.countryToString(selectedLocale.country());
#endif
        languageLabel->setText(QCoreApplication::translate("QtGui::QtLanguageOptionPage", "recognized by Qt as") % QStringLiteral(" <i>")
            % currentLocale.languageToString(selectedLocale.language()) % QChar(',') % QChar(' ') % territory % QStringLiteral("</i>"));
    });
    return widget;
}

QtEnvOptionPage::QtEnvOptionPage(QtSettingsData &settings, QWidget *parentWidget)
    : QtEnvOptionPageBase(parentWidget)
    , m_settings(settings)
{
}

QtEnvOptionPage::~QtEnvOptionPage()
{
}

bool QtEnvOptionPage::apply()
{
    m_settings.additionalPluginDirectory = ui()->pluginPathSelection->lineEdit()->text();
    m_settings.additionalIconThemeSearchPath = ui()->iconThemeSearchPathSelection->lineEdit()->text();
    TranslationFiles::additionalTranslationFilePath() = ui()->translationPathSelection->lineEdit()->text();
    return true;
}

void QtEnvOptionPage::reset()
{
    ui()->pluginPathSelection->lineEdit()->setText(m_settings.additionalPluginDirectory);
    ui()->iconThemeSearchPathSelection->lineEdit()->setText(m_settings.additionalIconThemeSearchPath);
    ui()->translationPathSelection->lineEdit()->setText(TranslationFiles::additionalTranslationFilePath());
}

QWidget *QtEnvOptionPage::setupWidget()
{
    // call base implementation first, so ui() is available
    return QtEnvOptionPageBase::setupWidget();
}

/*!
 * \brief Returns a handle to the internal data.
 * \remarks
 * This is an opaque data structure. It can be used to construct option pages
 * like QtLanguageOptionPage.
 */
QtSettings::operator QtSettingsData &() const
{
    return *m_d.get();
}

} // namespace QtUtilities

INSTANTIATE_UI_FILE_BASED_OPTION_PAGE(QtAppearanceOptionPage)
INSTANTIATE_UI_FILE_BASED_OPTION_PAGE(QtLanguageOptionPage)
INSTANTIATE_UI_FILE_BASED_OPTION_PAGE(QtEnvOptionPage)
