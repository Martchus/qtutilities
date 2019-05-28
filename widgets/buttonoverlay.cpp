#include "./buttonoverlay.h"
#include "./iconbutton.h"

#include <QCursor>
#include <QHBoxLayout>
#include <QStyle>
#include <QStyleOption>
#include <QToolTip>
#include <QWidget>

#include <functional>

/*!
 * \namespace Widgets
 * \brief Provides a set of extended widgets such as ClearLineEdit and
 * ClearComboBox.
 */

namespace Widgets {

/*!
 * \class Widgets::ButtonOverlay
 * \brief The ButtonOverlay class is used to display buttons on top of other
 * widgets.
 *
 * The class creates a new layout manager and sets it to the widget which is
 * specified
 * when constructing an instance. Thus this widget must not already have a
 * layout manager.
 *
 * The class is used to implement widget customization like ClearLineEidt and
 * ClearComboBox.
 */

/*!
 * \brief Constructs a button overlay for the specified \a widget.
 * \param widget Specifies the widget to display the buttons on.
 */
ButtonOverlay::ButtonOverlay(QWidget *widget)
    : m_widget(widget)
    , m_buttonWidget(new QWidget(widget))
    , m_buttonLayout(new QHBoxLayout(m_buttonWidget))
    , m_clearButton(nullptr)
    , m_infoButton(nullptr)
{
    // setup button widget and layout
    const QMargins margins = widget->contentsMargins();
    QStyleOption opt;
    opt.initFrom(m_widget);
    const int frameWidth = widget->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &opt, m_widget);
    const int pad = 2;
    m_buttonLayout->setContentsMargins(
        margins.left() + frameWidth + pad, margins.top() + frameWidth, margins.right() + frameWidth + pad, margins.bottom() + frameWidth);
    m_buttonLayout->setAlignment(Qt::AlignCenter | Qt::AlignRight);
    widget->setLayout(m_buttonLayout);
}

/*!
 * \brief Destroys the button overlay.
 */
ButtonOverlay::~ButtonOverlay()
{
}

/*!
 * \brief Sets whether the clear button is enabled.
 */
void ButtonOverlay::setClearButtonEnabled(bool enabled)
{
    if (isClearButtonEnabled() && !enabled) {
        // disable clear button
        m_buttonLayout->removeWidget(m_clearButton);
        delete m_clearButton;
        m_clearButton = nullptr;
    } else if (!isClearButtonEnabled() && enabled) {
        // enable clear button
        m_clearButton = new IconButton;
        m_clearButton->setHidden(isCleared());
        m_clearButton->setPixmap(QIcon::fromTheme(QStringLiteral("edit-clear"),
            QIcon(QStringLiteral(":/qtutilities/icons/hicolor/48x48/actions/"
                                 "edit-clear.png")))
                                     .pixmap(16));
        m_clearButton->setGeometry(0, 0, 16, 16);
        m_clearButton->setToolTip(QObject::tr("Clear"));
        QObject::connect(m_clearButton, &IconButton::clicked, std::bind(&ButtonOverlay::handleClearButtonClicked, this));
        m_buttonLayout->addWidget(m_clearButton);
    }
}

/*!
 * \brief Shows an info button with the specified \a pixmap and \a infoText.
 *
 * If there is already an info button enabled, it gets replaced with the new
 * button.
 *
 * \sa ButtonOverlay::disableInfoButton()
 */
void ButtonOverlay::enableInfoButton(const QPixmap &pixmap, const QString &infoText)
{
    if (!m_infoButton) {
        m_infoButton = new IconButton;
        m_infoButton->setGeometry(0, 0, 16, 16);
        QObject::connect(m_infoButton, &IconButton::clicked, std::bind(&ButtonOverlay::showInfo, this));
        if (m_clearButton) {
            m_buttonLayout->insertWidget(m_buttonLayout->count() - 2, m_infoButton);
        } else {
            m_buttonLayout->addWidget(m_infoButton);
        }
    }
    m_infoButton->setPixmap(pixmap);
    m_infoButton->setToolTip(infoText);
}

/*!
 * \brief Hides an info button if one is shown.
 * \sa ButtonOverlay::enableInfoButton()
 */
void ButtonOverlay::disableInfoButton()
{
    if (m_infoButton) {
        m_buttonLayout->removeWidget(m_infoButton);
        delete m_infoButton;
        m_infoButton = nullptr;
    }
}

/*!
 * \brief Adds a custom \a button.
 *
 * The button overlay takes ownership over the specified \a button.
 */
void ButtonOverlay::addCustomButton(QWidget *button)
{
    m_buttonLayout->addWidget(button);
}

/*!
 * \brief Inserts a custom \a button at the specified \a index.
 *
 * The button overlay takes ownership over the specified \a button.
 */
void ButtonOverlay::insertCustomButton(int index, QWidget *button)
{
    m_buttonLayout->insertWidget(index, button);
}

/*!
 * \brief Removes the specified custom \a button.
 *
 * The ownership of widget remains the same as when it was added.
 */
void ButtonOverlay::removeCustomButton(QWidget *button)
{
    m_buttonLayout->removeWidget(button);
}

/*!
 * \brief Updates the visibility of the clear button.
 *
 * This method is meant to be called when subclassing.
 */
void ButtonOverlay::updateClearButtonVisibility(bool visible)
{
    if (m_clearButton) {
        m_clearButton->setVisible(visible);
    }
}

/*!
 * \brief Clears the related widget.
 *
 * This method is meant to be implemented when subclassing.
 */
void ButtonOverlay::handleClearButtonClicked()
{
}

/*!
 * \brief Returns whether the related widget is cleared.
 *
 * This method is meant to be implemented when subclassing.
 */
bool ButtonOverlay::isCleared() const
{
    return false;
}

/*!
 * \brief Shows the info text using a tool tip.
 *
 * This method is called when the info button is clicked.
 */
void ButtonOverlay::showInfo()
{
    if (m_infoButton) {
        QToolTip::showText(QCursor::pos(), m_infoButton->toolTip(), m_infoButton);
    }
}
} // namespace Widgets
