#include "./optioncategory.h"
#include "./optionpage.h"

namespace Dialogs {

/*!
 * \class Dialogs::OptionCategory
 * \brief The OptionCategory class wraps associated option pages.
 */

/*!
 * \brief Constructs a option category.
 */
OptionCategory::OptionCategory(QObject *parent)
    : QObject(parent)
    , m_currentIndex(0)
{
}

/*!
 * \brief Destroys the option category.
 */
OptionCategory::~OptionCategory()
{
    qDeleteAll(m_pages);
}

/*!
 * \brief Applies all pages.
 * \sa OptionPage::apply()
 */
bool OptionCategory::applyAllPages()
{
    for (OptionPage *page : m_pages) {
        if (!page->apply()) {
            return false;
        }
    }
    return true;
}

/*!
 * \brief Resets all pages.
 * \sa OptionPage::reset()
 */
void OptionCategory::resetAllPages()
{
    for (OptionPage *page : m_pages) {
        page->reset();
    }
}

/*!
 * \brief Returns whether the option category matches the specified \a searchKeyWord.
 */
bool OptionCategory::matches(const QString &searchKeyWord) const
{
    for (OptionPage *page : m_pages) {
        if (page->matches(searchKeyWord)) {
            return true;
        }
    }
    return false;
}

/*!
 * \brief Assigns the specified \a pages to the category.
 *
 * Previously assigned pages get deleted. The pagesChanged() signal is emitted.
 * The category takes ownership over the given \a pages.
 */
void OptionCategory::assignPages(const QList<OptionPage *> pages)
{
    qDeleteAll(m_pages);
    m_pages = pages;
    emit pagesChanged();
}

/*!
 * \fn OptionCategory::displayNameChanged()
 * \brief Emitted when the display name changed.
 */

/*!
 * \fn OptionCategory::iconChanged()
 * \brief Emitted when the icon changed.
 */

/*!
 * \fn OptionCategory::pagesChanged()
 * \brief Emitted when the pages changed.
 */
}
