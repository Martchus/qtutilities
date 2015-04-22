#include "optionpage.h"

#include <QLabel>
#include <QCheckBox>
#include <QRadioButton>
#include <QPushButton>
#include <QGroupBox>

namespace Dialogs {

/*!
 * \class Dialogs::OptionPage
 * \brief The OptionPage class is the base class for SettingsDialog pages.
 *
 * The specified \a parentWindow might be used by some implementations as parent when showing dialogs.
 */

/*!
 * \brief Constructs a option page.
 */
OptionPage::OptionPage(QWidget *parentWindow) :
    m_parentWindow(parentWindow),
    m_shown(false),
    m_keywordsInitialized(false)
{}

/*!
 * \brief Destroys the option page.
 */
OptionPage::~OptionPage()
{}

/*!
 * \brief Returns the widget for the option page.
 *
 * If the widget has not been constructed yet, a new widget will be
 * constructed using the OptionPage::setupWidget() method and the
 * current configuration is applied.
 *
 * The option page keeps ownership over the returned widget.
 */
QWidget *OptionPage::widget()
{
    if(!m_widget) {
        m_widget.reset(setupWidget()); // ensure widget has been created
    }
    if(!m_shown) {
        m_shown = true;
        reset(); // show current configuration if not shown yet
    }
    return m_widget.get();
}

/*!
 * \brief Returns whether the pages matches the specified
 *        \a searchKeyWord.
 */
bool OptionPage::matches(const QString &searchKeyWord)
{
    if(searchKeyWord.isEmpty()) {
        return true;
    }
    if(displayName().contains(searchKeyWord, Qt::CaseInsensitive)) {
        return true;
    }
    if(!m_keywordsInitialized) {
        if(!m_widget) {
            m_widget.reset(setupWidget()); // ensure widget has been created
        }
        // find common subwidgets
        foreach (const QLabel *label, m_widget->findChildren<QLabel *>())
            m_keywords << label->text();
        foreach (const QCheckBox *checkbox, m_widget->findChildren<QCheckBox *>())
            m_keywords << checkbox->text();
        foreach (const QRadioButton *checkbox, m_widget->findChildren<QRadioButton *>())
            m_keywords << checkbox->text();
        foreach (const QPushButton *pushButton, m_widget->findChildren<QPushButton *>())
            m_keywords << pushButton->text();
        foreach (const QGroupBox *groupBox, m_widget->findChildren<QGroupBox *>())
            m_keywords << groupBox->title();
        m_keywordsInitialized = true;
    }
    foreach (const QString &keyword, m_keywords)
        if (keyword.contains(searchKeyWord, Qt::CaseInsensitive))
            return true;
    return false;
}

/*!
 * \fn OptionPage::displayName()
 * \brief Returns the display name of the page.
 */

/*!
 * \fn OptionPage::apply()
 * \brief Applies altered settings.
 */

/*!
 * \fn OptionPage::reset()
 * \brief Discards altered settings and resets relevant widgets.
 */

/*!
 * \fn OptionPage::setupWidget()
 * \brief Creates the widget for the page. Called in the first invocation of widget().
 */

}
