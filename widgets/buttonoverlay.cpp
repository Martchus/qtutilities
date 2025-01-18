#include "./buttonoverlay.h"
#include "./iconbutton.h"

#include <QAction>
#include <QComboBox>
#include <QCursor>
#include <QHBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QStyle>
#include <QStyleOption>
#include <QToolTip>
#include <QWidget>

#include <functional>

namespace QtUtilities {

/*!
 * \class ButtonOverlay
 * \brief The ButtonOverlay class is used to display buttons on top of other widgets.
 *
 * This class had been created before QLineEdit's functions setClearButtonEnabled() and
 * addAction() have been available. (These functions have been available only since Qt 5.2.)
 *
 * The downside of the "custom approach" compared to QLineEdit's own functions is that the
 * buttons are shown over the text as the text margins are not updated accordingly. Hence
 * the ButtonOverlay class has been updated to use QLineEdit's functions internally when the
 * specified widget is QLineEdit-based and its QLineEdit has been passed to the constructor.
 * However, when using any functions which can not be implemented using QLineEdit's own
 * functions, the ButtonOverlay has to fallback to its "custom approach". All functions which
 * cause this have a remark in their documentation.
 *
 * When QLineEdit's functions can not be used, the ButtonOverlay class creates a new layout
 * manager and sets it to the widget specified when constructing an instance. Thus this widget
 * must not already have a layout manager.
 *
 * The class is used to implement widget customization like ClearLineEidt and ClearComboBox
 * and most of the times it makes sense to use these widgets instead of using ButtonOverlay
 * directly.
 */

/*!
 * \brief Constructs a button overlay for the specified \a widget.
 * \param widget Specifies the widget to display the buttons on.
 * \remarks This function enforces the "custom approach" mentioned in the class documentation
 *          and should therefore be avoided.
 */
ButtonOverlay::ButtonOverlay(QWidget *widget)
    : m_widget(widget)
    , m_buttonWidget(nullptr)
    , m_buttonLayout(nullptr)
    , m_clearButton(nullptr)
    , m_infoButtonOrAction(nullptr)
{
    fallbackToUsingCustomLayout();
}

/*!
 * \brief Constructs a button overlay for the specified \a widget.
 * \param widget Specifies the widget to display the buttons on.
 * \param lineEdit Specifies the line edit used by \a widget to use the QLineEdit's functions
 *                 for adding actions instead of a custom layout.
 */
ButtonOverlay::ButtonOverlay(QWidget *widget, QLineEdit *lineEdit)
    : m_widget(widget)
    , m_buttonWidget(lineEdit)
    , m_buttonLayout(nullptr)
    , m_clearButton(nullptr)
    , m_infoButtonOrAction(nullptr)
{
    if (!m_buttonWidget) {
        fallbackToUsingCustomLayout();
    }
}

/*!
 * \brief Destroys the button overlay.
 */
ButtonOverlay::~ButtonOverlay()
{
}

/*!
 * \brief Returns whether the "custom approach" mentioned in the class documentation is used.
 */
bool ButtonOverlay::isUsingCustomLayout() const
{
    return m_buttonLayout != nullptr;
}

/*!
 * \brief Returns the layout manager holding the buttons.
 * \remarks This function enforces the "custom approach" mentioned in the class documentation
 *          and should therefore be avoided.
 */
QHBoxLayout *ButtonOverlay::buttonLayout()
{
    fallbackToUsingCustomLayout();
    return m_buttonLayout;
}

/*!
 * \brief Returns whether the clear button is enabled.
 */
bool ButtonOverlay::isClearButtonEnabled() const
{
    if (isUsingCustomLayout()) {
        return m_clearButton != nullptr;
    }
    return lineEditForWidget()->isClearButtonEnabled();
}

/*!
 * \brief Returns whether the info button is enabled.
 */
bool ButtonOverlay::isInfoButtonEnabled() const
{
    return m_infoButtonOrAction != nullptr;
}

/*!
 * \brief Sets whether the clear button is enabled.
 */
void ButtonOverlay::setClearButtonEnabled(bool enabled)
{
    if (auto *const le = lineEditForWidget()) {
        le->setClearButtonEnabled(enabled);
        return;
    }
    const auto clearButtonEnabled = isClearButtonEnabled();
    if (clearButtonEnabled && !enabled) {
        // disable clear button
        m_buttonLayout->removeWidget(m_clearButton);
        delete m_clearButton;
        m_clearButton = nullptr;
    } else if (!clearButtonEnabled && enabled) {
        // enable clear button
        m_clearButton = new IconButton;
        m_clearButton->setHidden(isCleared());
        m_clearButton->setPixmap(QIcon::fromTheme(QStringLiteral("edit-clear")).pixmap(IconButton::defaultPixmapSize));
        m_clearButton->setGeometry(QRect(QPoint(), IconButton::defaultPixmapSize));
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
    if (auto *const le = lineEditForWidget()) {
        disableInfoButton();
        auto *const action = le->addAction(QIcon(pixmap), QLineEdit::TrailingPosition);
        action->setToolTip(infoText);
        QObject::connect(action, &QAction::triggered, std::bind(&ButtonOverlay::showInfo, this));
        m_infoButtonOrAction = action;
        return;
    }
    auto *infoButton = static_cast<IconButton *>(m_infoButtonOrAction);
    if (!infoButton) {
        m_infoButtonOrAction = infoButton = new IconButton;
        infoButton->setGeometry(QRect(QPoint(), IconButton::defaultPixmapSize));
        if (m_clearButton) {
            m_buttonLayout->insertWidget(m_buttonLayout->count() - 2, infoButton);
        } else {
            m_buttonLayout->addWidget(infoButton);
        }
    }
    infoButton->setPixmap(pixmap);
    infoButton->setToolTip(infoText);
}

/*!
 * \brief Hides an info button if one is shown.
 * \sa ButtonOverlay::enableInfoButton()
 */
void ButtonOverlay::disableInfoButton()
{
    if (auto *const le = lineEditForWidget()) {
        if (auto *const infoAction = static_cast<QAction *>(m_infoButtonOrAction)) {
            le->removeAction(infoAction);
            m_infoButtonOrAction = nullptr;
        }
        return;
    }
    if (auto *infoButton = static_cast<IconButton *>(m_infoButtonOrAction)) {
        m_buttonLayout->removeWidget(infoButton);
        delete infoButton;
        m_infoButtonOrAction = nullptr;
    }
}

/*!
 * \brief Adds a custom \a button.
 *
 * The button overlay takes ownership over the specified \a button.
 *
 * \remarks This function enforces the "custom approach" mentioned in the class documentation
 *          and should therefore be avoided.
 */
void ButtonOverlay::addCustomButton(QWidget *button)
{
    fallbackToUsingCustomLayout();
    m_buttonLayout->addWidget(button);
}

/*!
 * \brief Inserts a custom \a button at the specified \a index.
 *
 * The button overlay takes ownership over the specified \a button.
 *
 * \remarks This function enforces the "custom approach" mentioned in the class documentation
 *          and should therefore be avoided.
 */
void ButtonOverlay::insertCustomButton(int index, QWidget *button)
{
    fallbackToUsingCustomLayout();
    m_buttonLayout->insertWidget(index, button);
}

/*!
 * \brief Removes the specified custom \a button; does nothing if \a button has not been added.
 *
 * The ownership of widget remains the same as when it was added.
 */
void ButtonOverlay::removeCustomButton(QWidget *button)
{
    if (isUsingCustomLayout()) {
        m_buttonLayout->removeWidget(button);
    }
}

/*!
 * \brief Adds a custom \a action.
 */
void ButtonOverlay::addCustomAction(QAction *action)
{
    if (auto *const le = lineEditForWidget()) {
        le->addAction(action, QLineEdit::TrailingPosition);
    } else {
        addCustomButton(IconButton::fromAction(action, reinterpret_cast<std::uintptr_t>(this)));
    }
}

/*!
 * \brief Inserts a custom \a action at the specified \a index.
 */
void ButtonOverlay::insertCustomAction(int index, QAction *action)
{
    if (auto *const le = lineEditForWidget()) {
        const auto actions = le->actions();
        le->insertAction(index < actions.size() ? actions[index] : nullptr, action);
    } else {
        insertCustomButton(index, IconButton::fromAction(action, reinterpret_cast<std::uintptr_t>(this)));
    }
}

/*!
 * \brief Removes the specified custom \a action; does nothing if \a action has not been added.
 */
void ButtonOverlay::removeCustomAction(QAction *action)
{
    if (auto *const le = lineEditForWidget()) {
        le->removeAction(action);
    } else {
        removeCustomButton(IconButton::fromAction(action, reinterpret_cast<std::uintptr_t>(this)));
    }
}

/*!
 * \brief Updates the visibility of the clear button.
 *
 * This function is meant to be called when subclassing.
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
 * This function is meant to be implemented when subclassing to support the clear button.
 */
void ButtonOverlay::handleClearButtonClicked()
{
}

/*!
 * \brief Applies additional handling when the button layout has been created.
 *
 * This function is meant to be implemented when subclassing when additional handling is
 * required.
 */
void ButtonOverlay::handleCustomLayoutCreated()
{
}

/*!
 * \brief Switches to the "custom approach".
 * \remarks This function is internally used when any legacy function is called
 *          or when the QLineEdit for the specified widget can not be determined.
 */
void ButtonOverlay::fallbackToUsingCustomLayout()
{
    // skip if custom layout is already used
    if (isUsingCustomLayout()) {
        return;
    }

    // disable QLineEdit's clear button and actions; save configuration
    auto clearButtonEnabled = false;
    auto *iconAction = static_cast<QAction *>(m_infoButtonOrAction);
    QPixmap infoPixmap;
    QString infoText;
    QList<QAction *> actions;
    if (auto *const le = lineEditForWidget()) {
        if ((clearButtonEnabled = le->isClearButtonEnabled())) {
            setClearButtonEnabled(false);
        }
        if ((iconAction = static_cast<QAction *>(m_infoButtonOrAction))) {
            const auto icon = iconAction->icon();
            const auto sizes = icon.availableSizes();
            infoPixmap = icon.pixmap(sizes.empty() ? IconButton::defaultPixmapSize : sizes.front());
            infoText = iconAction->toolTip();
            disableInfoButton();
        }
        actions = le->actions();
        for (auto *const action : actions) {
            le->removeAction(action);
        }
    }

    // initialize custom layout
    m_buttonLayout = new QHBoxLayout(m_buttonWidget);
    m_buttonWidget = new QWidget(m_widget);
    m_buttonLayout->setAlignment(Qt::AlignCenter | Qt::AlignRight);
    m_widget->setLayout(m_buttonLayout);
    handleCustomLayoutCreated();

    // restore old configuration
    if (clearButtonEnabled) {
        setClearButtonEnabled(true);
    }
    if (iconAction) {
        enableInfoButton(infoPixmap, infoText);
    }
    for (auto *const action : actions) {
        addCustomAction(action);
    }
}

/*!
 * \brief Returns the QLineEdit used to implement the button overlay.
 * \remarks This is always nullptr in case the "custom approach" is used.
 */
QLineEdit *ButtonOverlay::lineEditForWidget() const
{
    return isUsingCustomLayout() ? nullptr : static_cast<QLineEdit *>(m_buttonWidget);
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
 *
 * \remarks
 * This function avoids using QCursor::pos() because it is problematic to use under Wayland. For the action case it seems not
 * possible to avoid it because the position of QLineEditIconButton used by QLineEdit is not exposed.
 */
void ButtonOverlay::showInfo()
{
    if (lineEditForWidget()) {
        if (auto *const infoAction = static_cast<QAction *>(m_infoButtonOrAction)) {
            const auto pos = QCursor::pos();
            if (!pos.isNull()) {
                QToolTip::showText(pos, infoAction->toolTip(), m_widget);
            }
        }
        return;
    }
    if (auto *const infoButton = static_cast<IconButton *>(m_infoButtonOrAction)) {
        QToolTip::showText(infoButton->mapToGlobal(infoButton->rect().center()), infoButton->toolTip(), infoButton);
    }
}

/*!
 * \brief Sets the contents margins of the button layout so the overlay buttons will only be shown over the \a editFieldRect and
 *        not interfere with e.g. spin box buttons.
 * \remarks This function enforces the "custom approach" mentioned in the class documentation
 *          and should therefore be avoided. Of course it makes sense to call it within handleCustomLayoutCreated().
 */
void ButtonOverlay::setContentsMarginsFromEditFieldRectAndFrameWidth(const QRect &editFieldRect, int frameWidth, int padding)
{
    const auto margins = m_widget->contentsMargins();
    const auto buttonWidth = m_widget->width() - editFieldRect.width();
    buttonLayout()->setContentsMargins(margins.left() + frameWidth + padding, margins.top() + frameWidth,
        margins.right() + frameWidth + padding + buttonWidth, margins.bottom() + frameWidth);
}

} // namespace QtUtilities
