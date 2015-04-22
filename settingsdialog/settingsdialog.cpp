#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "optioncategorymodel.h"
#include "optioncategoryfiltermodel.h"
#include "optioncategory.h"
#include "optionpage.h"

#include <QItemSelectionModel>
#include <QShowEvent>
#include <QScrollArea>

namespace Dialogs {

/*!
 * \class Dialogs::SettingsDialog
 * \brief The SettingsDialog class provides a framework for creating settings dialogs with different categories and subcategories.
 */

/*!
 * \brief Constructs a settings dialog.
 * \param parent Specifies the parent widget.
 */
SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::SettingsDialog),
    m_categoryModel(new OptionCategoryModel(this)),
    m_categoryFilterModel(new OptionCategoryFilterModel(this)),
    m_currentCategory(nullptr),
    m_tabBarAlwaysVisible(true)
{
    m_ui->setupUi(this);
#ifdef Q_OS_WIN32
    setStyleSheet(QStringLiteral("* { font: 9pt \"Segoe UI\"; } #mainWidget { color: black; background-color: white; border: none; } #bottomWidget { background-color: #F0F0F0; border-top: 1px solid #DFDFDF; } QMessageBox QLabel, QInputDialog QLabel, #instructionLabel {font-size: 12pt; color: #003399; }"));
#else
    setStyleSheet(QStringLiteral("#instructionLabel { font-weight: bold; font-size: 12pt; }"));
#endif
    // setup models
    m_categoryFilterModel->setSourceModel(m_categoryModel);
    m_ui->categoriesListView->setModel(m_categoryFilterModel);
    // connect signals and slots
    //  selection models
    connect(m_ui->categoriesListView->selectionModel(), &QItemSelectionModel::currentChanged, this, &SettingsDialog::currentCategoryChanged);
    //  buttons
    connect(m_ui->abortPushButton, &QPushButton::clicked, this, &SettingsDialog::reject);
    connect(m_ui->applyPushButton, &QPushButton::clicked, this, &SettingsDialog::apply);
    connect(m_ui->okPushButton, &QPushButton::clicked, this, &SettingsDialog::accept);
    //  dialog
    connect(this, &SettingsDialog::accepted, this, &SettingsDialog::apply);
    connect(this, &SettingsDialog::rejected, this, &SettingsDialog::reset);
    //  misc
    connect(m_ui->filterLineEdit, &QLineEdit::textChanged, m_categoryFilterModel, &OptionCategoryFilterModel::setFilterFixedString);
    connect(m_ui->filterLineEdit, &QLineEdit::textChanged, this, &SettingsDialog::updateTabWidget);
}

/*!
 * \brief Destroys the settings dialog.
 */
SettingsDialog::~SettingsDialog()
{}

/*!
 * \brief Sets whether the tab bar is always visible.
 *
 * \sa SettingsDialog::isTabBarAlwaysVisible()
 */
void SettingsDialog::setTabBarAlwaysVisible(bool value)
{
    m_tabBarAlwaysVisible = value;
    if(m_currentCategory) {
        m_ui->pagesTabWidget->tabBar()->setHidden(!value && m_currentCategory->pages().size() == 1);
    }
}

/*!
 * \brief Returns the category for the specified \a categoryIndex.
 *
 * The settings dialog keeps ownership over the returned category.
 * If no category exists for the specified index a null pointer is returned.
 */
OptionCategory *SettingsDialog::category(int categoryIndex) const
{
    return m_categoryModel->category(categoryIndex);
}

/*!
 * \brief Returns the page for the specified \a categoryIndex and the specified \a pageIndex.
 *
 * The settings dialog keeps ownership over the returned category.
 * If no page for the specified indices a null pointer is returned.
 */
OptionPage *SettingsDialog::page(int categoryIndex, int pageIndex) const
{
    if(OptionCategory *category = this->category(categoryIndex)) {
        if(pageIndex < category->pages().length()) {
            return category->pages()[pageIndex];
        }
    }
    return nullptr;
}

/*!
 * \brief Resets all pages before the dialog is shown by the application.
 */
void SettingsDialog::showEvent(QShowEvent *event)
{
    if(!event->spontaneous()) {
        foreach(OptionCategory *category, m_categoryModel->categories()) {
            foreach(OptionPage *page, category->pages()) {
                page->reset();
            }
        }
    }
}

/*!
 * \brief Shows the selected category specified by its model \a index in the category model.
 *
 * This private slot is called when m_ui->categoriesListView->selectionModel()->currentChanged() is emitted.
 */
void SettingsDialog::currentCategoryChanged(const QModelIndex &index)
{
    showCategory(m_categoryModel->category(m_categoryFilterModel->mapToSource(index)));
}

/*!
 * \brief Sets the current category to the specified \a category and updates the relevant widgets to show it.
 */
void SettingsDialog::showCategory(OptionCategory *category)
{
    if(category) {
        if(m_currentCategory != category) {
            m_currentCategory = category;
            m_ui->instructionLabel->setText(category->displayName());
        }
    } else {
        m_currentCategory = nullptr;
        m_ui->instructionLabel->setText(tr("No category selected"));
    }
    updateTabWidget();
}

/*!
 * \brief Updates the tab widget to show the pages for the current category.
 */
void SettingsDialog::updateTabWidget()
{
    if(m_currentCategory) {
        m_ui->pagesTabWidget->setUpdatesEnabled(false);
        QString searchKeyWord = m_ui->filterLineEdit->text();
        int index = 0;
        foreach(OptionPage *page, m_currentCategory->pages()) {
            if(page->matches(searchKeyWord)) {
                QScrollArea *scrollArea;
                if(index < m_ui->pagesTabWidget->count()) {
                    scrollArea = qobject_cast<QScrollArea *>(m_ui->pagesTabWidget->widget(index));
                    scrollArea->takeWidget();
                    m_ui->pagesTabWidget->setTabText(index, page->displayName());
                } else {
                    scrollArea = new QScrollArea(m_ui->pagesTabWidget);
                    scrollArea->setFrameStyle(QFrame::NoFrame);
                    scrollArea->setBackgroundRole(QPalette::Base);
                    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                    scrollArea->setWidgetResizable(true);
                    m_ui->pagesTabWidget->addTab(scrollArea, page->displayName());
                }
                if(page->widget()->layout()) {
                    page->widget()->layout()->setAlignment(Qt::AlignTop | Qt::AlignLeft);
                }
                scrollArea->setWidget(page->widget());
            }
            ++index;
        }
        while(index < m_ui->pagesTabWidget->count()) {
            QScrollArea *scrollArea = qobject_cast<QScrollArea *>(m_ui->pagesTabWidget->widget(index));
            scrollArea->takeWidget();
            m_ui->pagesTabWidget->removeTab(index);
            delete scrollArea;
        }
        m_ui->pagesTabWidget->tabBar()->setHidden(!m_tabBarAlwaysVisible && m_ui->pagesTabWidget->count() == 1);
        m_ui->pagesTabWidget->setUpdatesEnabled(true);
    } else {
        m_ui->pagesTabWidget->clear();
    }
}

/*!
 * \brief Applies all changes. Calls OptionCategory::applyAllPages() for each category.
 */
bool SettingsDialog::apply()
{
    foreach(OptionCategory *category, m_categoryModel->categories()) {
        foreach(OptionPage *page, category->pages()) {
            if(!page->apply()) {
                return false;
            }
            category->applyAllPages();
        }
    }
    emit applied();
    return true;
}

/*!
 * \brief Resets all changes. Calls OptionCategory::resetAllPages() for each category.
 */
void SettingsDialog::reset()
{
    foreach(OptionCategory *category, m_categoryModel->categories()) {
        category->resetAllPages();
    }
    emit resetted();
}

}
