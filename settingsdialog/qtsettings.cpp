#include "./qtsettings.h"

#include "./optioncategorymodel.h"
#include "./optioncategoryfiltermodel.h"
#include "./optioncategory.h"
#include "./optionpage.h"

#include "../paletteeditor/paletteeditor.h"

#include "../widgets/clearlineedit.h"

#include "ui_qtappearanceoptionpage.h"
#include "ui_qtlanguageoptionpage.h"
#include "ui_qtenvoptionpage.h"

#include <QStyleFactory>
#include <QFontDialog>
#include <QFileDialog>
#include <QTimer>

#include <iostream>

using namespace std;

namespace Dialogs {

/*!
 * \brief Returns a new OptionCatecory containing all Qt related
 *        option pages.
 */
OptionCategory *qtOptionCategory(QObject *parent)
{
    auto *category = new OptionCategory(parent);
    category->setDisplayName(QCoreApplication::translate("QtGui::QtOptionCategory", "Qt"));
    category->setIcon(QIcon::fromTheme(QStringLiteral("qtcreator")));
    category->assignPages(QList<OptionPage *>()
                          << new QtAppearanceOptionPage
                          << new QtLanguageOptionPage
                          << new QtEnvOptionPage);
    return category;
}

QtAppearanceOptionPage::QtAppearanceOptionPage(QWidget *parentWidget) :
    QtAppearanceOptionPageBase(parentWidget),
    m_fontDialog(nullptr)
{}

QtAppearanceOptionPage::~QtAppearanceOptionPage()
{}

bool QtAppearanceOptionPage::apply()
{
    if(hasBeenShown()) {
        // read style sheet
        QString styleSheet;
        if(!ui()->styleSheetPathSelection->lineEdit()->text().isEmpty()) {
            QFile file(ui()->styleSheetPathSelection->lineEdit()->text());
            if(!file.open(QFile::ReadOnly)) {
                return false;
            }
            styleSheet.append(file.readAll());
            if(file.error() != QFile::NoError) {
                return false;
            }
        }

        // apply config
        const QFont font = ui()->fontComboBox->font();
        const QPalette palette = ui()->paletteToolButton->palette();
        QGuiApplication::setFont(font);
        QGuiApplication::setPalette(palette);
        if(auto *qapp = qobject_cast<QApplication *>(QApplication::instance())) {
            qapp->setStyleSheet(styleSheet);
        } else {
            return false;
        }
        QApplication::setStyle(ui()->widgetStyleComboBox->currentText());
    }
    return true;
}

void QtAppearanceOptionPage::reset()
{
    if(hasBeenShown()) {
        ui()->widgetStyleComboBox->setCurrentText(QApplication::style() ? QApplication::style()->objectName() : QString());
        ui()->styleSheetPathSelection->lineEdit()->setText(QString() /* TODO */);
        ui()->fontComboBox->setCurrentFont(QGuiApplication::font());
        ui()->fontPushButton->setPalette(QGuiApplication::palette());
    }
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
    QObject::connect(ui()->fontPushButton, &QPushButton::clicked, [this] {
        if(!m_fontDialog) {
            m_fontDialog = new QFontDialog(this->widget());
            m_fontDialog->setCurrentFont(ui()->fontComboBox->font());
            QObject::connect(m_fontDialog, &QFontDialog::fontSelected, ui()->fontComboBox, &QFontComboBox::setCurrentFont);
            QObject::connect(ui()->fontComboBox, &QFontComboBox::currentFontChanged, m_fontDialog, &QFontDialog::setCurrentFont);
        }
        m_fontDialog->show();
    });

    // setup palette selection
    QObject::connect(ui()->paletteToolButton, &QToolButton::clicked, [this] {
        ui()->paletteToolButton->setPalette(PaletteEditor::getPalette(this->widget(), ui()->paletteToolButton->palette()));
    });

    return widget;
}

QtLanguageOptionPage::QtLanguageOptionPage(QWidget *parentWidget) :
    QtLanguageOptionPageBase(parentWidget)
{}

QtLanguageOptionPage::~QtLanguageOptionPage()
{}

bool QtLanguageOptionPage::apply()
{
    if(hasBeenShown()) {
        QLocale::setDefault(ui()->localeComboBox->currentText());
    }
    return true;
}

void QtLanguageOptionPage::reset()
{
    if(hasBeenShown()) {
        ui()->localeComboBox->setCurrentText(QLocale().name());
    }
}

QtEnvOptionPage::QtEnvOptionPage(QWidget *parentWidget) :
    QtEnvOptionPageBase(parentWidget)
{}

QtEnvOptionPage::~QtEnvOptionPage()
{}

bool QtEnvOptionPage::apply()
{
    if(hasBeenShown()) {

    }
    return true;
}

void QtEnvOptionPage::reset()
{
    if(hasBeenShown()) {

    }
}

}

INSTANTIATE_UI_FILE_BASED_OPTION_PAGE(QtAppearanceOptionPage)
INSTANTIATE_UI_FILE_BASED_OPTION_PAGE(QtLanguageOptionPage)
INSTANTIATE_UI_FILE_BASED_OPTION_PAGE(QtEnvOptionPage)
