#include "./settingsdialog.h"

#include "./optioncategory.h"
#include "./optioncategoryfiltermodel.h"
#include "./optioncategorymodel.h"
#include "./optionpage.h"

#include "../misc/dialogutils.h"

#include "ui_settingsdialog.h"

#include <QItemSelectionModel>
#include <QMessageBox>
#include <QScrollArea>
#include <QShowEvent>
#include <QStringBuilder>

namespace QtUtilities {

/*!
 * \class SettingsDialog
 * \brief The SettingsDialog class provides a framework for creating settings
 * dialogs with different categories and subcategories.
 */

/*!
 * \brief Constructs a settings dialog.
 * \param parent Specifies the parent widget.
 */
SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::SettingsDialog)
    , m_categoryModel(new OptionCategoryModel(this))
    , m_categoryFilterModel(new OptionCategoryFilterModel(this))
    , m_currentCategory(nullptr)
    , m_tabBarAlwaysVisible(true)
{
    m_ui->setupUi(this);
    makeHeading(m_ui->headingLabel);
    setStyleSheet(dialogStyle());

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
{
}

/*!
 * \brief Sets whether the tab bar is always visible.
 * \sa SettingsDialog::isTabBarAlwaysVisible()
 */
void SettingsDialog::setTabBarAlwaysVisible(bool value)
{
    m_tabBarAlwaysVisible = value;
    if (m_currentCategory) {
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
 * \brief Returns the page for the specified \a categoryIndex and the specified
 * \a pageIndex.
 *
 * The settings dialog keeps ownership over the returned category.
 * If no page for the specified indices a null pointer is returned.
 */
OptionPage *SettingsDialog::page(int categoryIndex, int pageIndex) const
{
    if (OptionCategory *const category = this->category(categoryIndex)) {
        if (pageIndex < category->pages().length()) {
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
    if (event->spontaneous()) {
        return;
    }
    for (OptionCategory *const category : m_categoryModel->categories()) {
        for (OptionPage *const page : category->pages()) {
            if (page->hasBeenShown()) {
                page->reset();
            }
        }
    }
}

/*!
 * \brief Shows the selected category specified by its model \a index in the
 * category filter model.
 *
 * This private slot is called when
 * m_ui->categoriesListView->selectionModel()->currentChanged() is emitted.
 */
void SettingsDialog::currentCategoryChanged(const QModelIndex &index)
{
    showCategory(m_categoryModel->category(m_categoryFilterModel->mapToSource(index)));
}

/*!
 * \brief Sets the current category to the specified \a category and updates the
 * relevant widgets to show it.
 */
void SettingsDialog::showCategory(OptionCategory *category)
{
    if (m_currentCategory) {
        m_currentCategory->setCurrentIndex(m_ui->pagesTabWidget->currentIndex());
    }
    if (category) {
        if (m_currentCategory != category) {
            m_currentCategory = category;
            m_ui->headingLabel->setText(category->displayName());
        }
    } else {
        m_currentCategory = nullptr;
        m_ui->headingLabel->setText(tr("No category selected"));
    }
    updateTabWidget();
}

/*!
 * \brief Enables *single-category mode* to show only the specified \a
 * singleCategory.
 * \remarks
 * - In *single-category mode* category selection, filter and heading are
 * hidden.
 * - The *single-category mode* can be disabled again by setting \a
 * singleCategory to nullptr.
 */
void SettingsDialog::setSingleCategory(OptionCategory *singleCategory)
{
    const bool hasSingleCategory = singleCategory != nullptr;
    m_ui->filterLineEdit->setHidden(hasSingleCategory);
    m_ui->categoriesListView->setHidden(hasSingleCategory);
    m_ui->headingLabel->setHidden(hasSingleCategory);
    if (hasSingleCategory) {
        m_ui->filterLineEdit->clear();
        categoryModel()->setCategories(QList<OptionCategory *>({ singleCategory }));
        showCategory(singleCategory);
    }
}

/*!
 * \brief Updates the tab widget to show the pages for the current category.
 */
void SettingsDialog::updateTabWidget()
{
    if (!m_currentCategory) {
        m_ui->pagesTabWidget->clear();
        return;
    }
    m_ui->pagesTabWidget->setUpdatesEnabled(false);
    const QString searchKeyWord = m_ui->filterLineEdit->text();
    int index = 0, pageIndex = 0;
    for (OptionPage *const page : m_currentCategory->pages()) {
        if (page->matches(searchKeyWord)) {
            QScrollArea *scrollArea;
            if (index < m_ui->pagesTabWidget->count()) {
                scrollArea = qobject_cast<QScrollArea *>(m_ui->pagesTabWidget->widget(index));
                scrollArea->takeWidget();
                m_ui->pagesTabWidget->setTabText(index, page->widget()->windowTitle());
                m_ui->pagesTabWidget->setTabIcon(index, page->widget()->windowIcon());
            } else {
                scrollArea = new QScrollArea(m_ui->pagesTabWidget);
                scrollArea->setFrameStyle(QFrame::NoFrame);
                scrollArea->setBackgroundRole(QPalette::Base);
                scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                scrollArea->setWidgetResizable(true);
                m_ui->pagesTabWidget->addTab(scrollArea, page->widget()->windowTitle());
                m_ui->pagesTabWidget->setTabIcon(index, page->widget()->windowIcon());
            }
            if (page->widget()->layout()) {
                page->widget()->layout()->setAlignment(Qt::AlignTop | Qt::AlignLeft);
            }
            scrollArea->setWidget(page->widget());
            ++index;
        }
        if (pageIndex == m_currentCategory->currentIndex()) {
            m_ui->pagesTabWidget->setCurrentIndex(pageIndex);
        }
        ++pageIndex;
    }
    while (index < m_ui->pagesTabWidget->count()) {
        QScrollArea *const scrollArea = qobject_cast<QScrollArea *>(m_ui->pagesTabWidget->widget(index));
        scrollArea->takeWidget();
        m_ui->pagesTabWidget->removeTab(index);
        delete scrollArea;
    }
    m_ui->pagesTabWidget->tabBar()->setHidden(!m_tabBarAlwaysVisible && m_ui->pagesTabWidget->count() == 1);
    m_ui->pagesTabWidget->setUpdatesEnabled(true);
}

/*!
 * \brief Applies all changes. Calls OptionCategory::applyAllPages() for each category.
 * \remarks Pages which have not been shown yet must have not been initialized anyways
 *          and hence are skipped.
 */
bool SettingsDialog::apply()
{
    // apply each page in each category and gather error messages
    QString errorMessage;
    for (OptionCategory *const category : m_categoryModel->categories()) {
        for (OptionPage *const page : category->pages()) {
            if (!page->hasBeenShown() || page->apply()) {
                // nothing to apply or no error
                continue;
            }

            // add error message
            if (errorMessage.isEmpty()) {
                errorMessage = tr("<p><b>Errors occurred when applying changes:</b></p><ul>");
            }
            QStringList &errors = const_cast<OptionPage *>(page)->errors();
            if (errors.isEmpty()) {
                errorMessage.append(QStringLiteral("<li><i>") % category->displayName() % QLatin1Char('/') % page->widget()->windowTitle()
                    % QStringLiteral("</i>: ") % tr("unknown error") % QStringLiteral("</li>"));
            } else {
                for (const QString &error : errors) {
                    errorMessage.append(QStringLiteral("<li><i>") % category->displayName() % QLatin1Char('/') % page->widget()->windowTitle()
                        % QStringLiteral("</i>: ") % error % QStringLiteral("</li>"));
                }
                errors.clear();
            }
        }
    }

    // show error messages (if errors occurred)
    if (!errorMessage.isEmpty()) {
        errorMessage.append(QStringLiteral("</ul>"));
        QMessageBox::warning(this, windowTitle(), errorMessage);
    }

    // return status
    emit applied();
    return errorMessage.isEmpty();
}

/*!
 * \brief Resets all changes. Calls OptionCategory::resetAllPages() for each
 * category.
 */
void SettingsDialog::reset()
{
    for (OptionCategory *const category : m_categoryModel->categories()) {
        category->resetAllPages();
    }
    emit resetted();
}
} // namespace QtUtilities
