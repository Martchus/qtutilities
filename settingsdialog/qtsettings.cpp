#include "./qtsettings.h"
#include "./optioncategory.h"
#include "./optioncategoryfiltermodel.h"
#include "./optioncategorymodel.h"
#include "./optionpage.h"

#include "../paletteeditor/paletteeditor.h"

#include "../widgets/clearlineedit.h"

#include "../resources/resources.h"

#include "../misc/desktoputils.h"

#include "ui_qtappearanceoptionpage.h"
#include "ui_qtenvoptionpage.h"
#include "ui_qtlanguageoptionpage.h"

#include <QDir>
#include <QFileDialog>
#include <QFontDialog>
#include <QIcon>
#include <QSettings>
#include <QStringBuilder>
#include <QStyleFactory>

#include <iostream>
#include <memory>

using namespace std;

namespace QtUtilities {

struct QtSettingsData {
    QtSettingsData();

    QFont font;
    QPalette palette;
    QString widgetStyle;
    QString styleSheetPath;
    QString iconTheme;
    QLocale defaultLocale;
    QString localeName;
    QString additionalPluginDirectory;
    QString additionalIconThemeSearchPath;
    bool customFont;
    bool customPalette;
    bool customWidgetStyle;
    bool customStyleSheet;
    bool customIconTheme;
    bool customLocale;
    bool isPaletteDark;
};

inline QtSettingsData::QtSettingsData()
    : iconTheme(QIcon::themeName())
    , localeName(defaultLocale.name())
    , customFont(false)
    , customPalette(false)
    , customWidgetStyle(false)
    , customStyleSheet(false)
    , customIconTheme(false)
    , customLocale(false)
    , isPaletteDark(false)
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
    : m_d(make_unique<QtSettingsData>())
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
 *  - Some settings take only affect after restarting the application.
 *  - QApplication/QGuiApplication must be instantiated before calling this
 * method.
 *  - Hence it makes most sense to call this directly after instantiating
 * QApplication/QGuiApplication.
 */
void QtSettings::apply()
{
    // apply environment
    if (m_d->additionalPluginDirectory.isEmpty()) {
        QCoreApplication::addLibraryPath(m_d->additionalPluginDirectory);
    }
    if (!m_d->additionalIconThemeSearchPath.isEmpty()) {
        QIcon::setThemeSearchPaths(QIcon::themeSearchPaths() << m_d->additionalIconThemeSearchPath);
    }

    // read style sheet
    QString styleSheet;
    if (m_d->customStyleSheet && !m_d->styleSheetPath.isEmpty()) {
        QFile file(m_d->styleSheetPath);
        if (!file.open(QFile::ReadOnly)) {
            cerr << "Unable to open the specified stylesheet \"" << m_d->styleSheetPath.toLocal8Bit().data() << "\"." << endl;
        }
        styleSheet.append(file.readAll());
        if (file.error() != QFile::NoError) {
            cerr << "Unable to read the specified stylesheet \"" << m_d->styleSheetPath.toLocal8Bit().data() << "\"." << endl;
        }
    }

    // apply appearance
    if (m_d->customFont) {
        QGuiApplication::setFont(m_d->font);
    }
    if (m_d->customWidgetStyle) {
        QApplication::setStyle(m_d->widgetStyle);
    }
    if (!styleSheet.isEmpty()) {
        if (auto *qapp = qobject_cast<QApplication *>(QApplication::instance())) {
            qapp->setStyleSheet(styleSheet);
        } else {
            cerr << "Unable to apply the specified stylesheet \"" << m_d->styleSheetPath.toLocal8Bit().data()
                 << "\" because no QApplication has been instantiated." << endl;
        }
    }
    if (m_d->customPalette) {
        QGuiApplication::setPalette(m_d->palette);
    }
    m_d->isPaletteDark = isPaletteDark();
    if (m_d->customIconTheme) {
        QIcon::setThemeName(m_d->iconTheme);
    } else if (QIcon::themeName().isEmpty()) {
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
    QLocale::setDefault(m_d->customLocale ? QLocale(m_d->localeName) : m_d->defaultLocale);
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
    m_settings.palette = ui()->paletteToolButton->palette();
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
    ui()->paletteToolButton->setPalette(m_settings.palette);
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

    // setup widget style selection
    ui()->widgetStyleComboBox->addItems(QStyleFactory::keys());

    // setup style sheet selection
    ui()->styleSheetPathSelection->provideCustomFileMode(QFileDialog::ExistingFile);

    // setup font selection
    QObject::connect(ui()->fontPushButton, &QPushButton::clicked, m_fontDialog, [this] {
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
        [this] { ui()->paletteToolButton->setPalette(PaletteEditor::getPalette(this->widget(), ui()->paletteToolButton->palette())); });

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

    // add all available locales to combo box
    auto *localeComboBox = ui()->localeComboBox;
    const auto locales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry);
    for (const QLocale &locale : locales) {
        localeComboBox->addItem(locale.name());
    }

    auto *languageLabel = ui()->languageLabel;
    QObject::connect(ui()->localeComboBox, &QComboBox::currentTextChanged, languageLabel, [languageLabel, localeComboBox] {
        const QLocale selectedLocale(localeComboBox->currentText());
        const QLocale currentLocale;
        languageLabel->setText(QCoreApplication::translate("QtGui::QtLanguageOptionPage", "recognized by Qt as") % QStringLiteral(" <i>")
            % currentLocale.languageToString(selectedLocale.language()) % QChar(',') % QChar(' ')
            % currentLocale.countryToString(selectedLocale.country()) % QStringLiteral("</i>"));
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
} // namespace QtUtilities

INSTANTIATE_UI_FILE_BASED_OPTION_PAGE(QtAppearanceOptionPage)
INSTANTIATE_UI_FILE_BASED_OPTION_PAGE(QtLanguageOptionPage)
INSTANTIATE_UI_FILE_BASED_OPTION_PAGE(QtEnvOptionPage)
