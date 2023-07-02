#include "./optionpage.h"

#include <QCheckBox>
#include <QEvent>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>

#include <utility>

namespace QtUtilities {

/*!
 * \class OptionPage
 * \brief The OptionPage class is the base class for SettingsDialog pages.
 *
 * The specified \a parentWindow might be used by some implementations as parent
 * when showing dialogs.
 */

/*!
 * \brief Constructs a option page.
 */
OptionPage::OptionPage(QWidget *parentWindow)
    : m_parentWindow(parentWindow)
    , m_shown(false)
    , m_keywordsInitialized(false)
{
}

/*!
 * \brief Destroys the option page.
 */
OptionPage::~OptionPage()
{
}

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
    if (!m_widget) {
        m_widget.reset(setupWidget()); // ensure widget has been created
    }
    if (!m_shown) {
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
    if (searchKeyWord.isEmpty()) {
        return true;
    }
    if (!m_keywordsInitialized) {
        if (!m_widget) {
            m_widget.reset(setupWidget()); // ensure widget has been created
        }
        m_keywords << m_widget->windowTitle();
        // find common subwidgets
        for (const QLabel *label : m_widget->findChildren<QLabel *>())
            m_keywords << label->text();
        for (const QCheckBox *checkbox : m_widget->findChildren<QCheckBox *>())
            m_keywords << checkbox->text();
        for (const QRadioButton *checkbox : m_widget->findChildren<QRadioButton *>())
            m_keywords << checkbox->text();
        for (const QPushButton *pushButton : m_widget->findChildren<QPushButton *>())
            m_keywords << pushButton->text();
        for (const QGroupBox *groupBox : m_widget->findChildren<QGroupBox *>())
            m_keywords << groupBox->title();
        m_keywordsInitialized = true;
    }
    for (const QString &keyword : std::as_const(m_keywords))
        if (keyword.contains(searchKeyWord, Qt::CaseInsensitive))
            return true;
    return false;
}

/*!
 * \brief Emits the paletteChanged() signal.
 */
bool OptionPageWidget::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::PaletteChange:
        emit paletteChanged();
        break;
    case QEvent::LanguageChange:
        emit retranslationRequired();
        break;
    default:;
    }
    return QWidget::event(event);
}

/*!
 * \fn OptionPage::apply()
 * \brief Applies altered settings.
 * \remarks
 * The SettingsDialog and any other classes/functions of this library will not call
 * this method if the option page has not been shown yet. Hence it is (no longer) necessary
 * to use OptionPage::hasBeenShown() to check whether the page has been initialized
 * yet.
 */

/*!
 * \fn OptionPage::reset()
 * \brief Discards altered settings and resets relevant widgets.
 * \remarks
 * The SettingsDialog and any other classes/functions of this library will not call
 * this method if the option page has not been shown yet. Hence it is (no longer) necessary
 * to use OptionPage::hasBeenShown() to check whether the page has been initialized
 * yet.
 */

/*!
 * \fn OptionPage::setupWidget()
 * \brief Creates the widget for the page. Called on the first invocation of
 * widget().
 */
} // namespace QtUtilities
