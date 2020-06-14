#include "./iconbutton.h"

#include <c++utilities/conversion/stringbuilder.h>

#include <QKeyEvent>
#include <QStyle>
#include <QStyleOptionFocusRect>
#include <QStylePainter>

using namespace CppUtilities;

namespace QtUtilities {

/*!
 * \class IconButton
 * \brief A simple QAbstractButton implementation displaying a QPixmap.
 */

/*!
 * \brief Constructs an icon button.
 */
IconButton::IconButton(QWidget *parent)
    : QAbstractButton(parent)
{
    setCursor(Qt::ArrowCursor);
    setFocusPolicy(Qt::NoFocus);
}

/*!
 * \brief Destroys the icon button.
 */
IconButton::~IconButton()
{
}

/*!
 * \brief Creates an IconButton for the specified \a action.
 * \remarks Calling this function on the same action twice with the same \a id yields the
 *          same instance.
 */
IconButton *IconButton::fromAction(QAction *action, std::uintptr_t id)
{
    const auto propertyName = argsToString("iconButton-", id);
    const auto existingIconButton = action->property(propertyName.data());
    if (!existingIconButton.isNull()) {
        return existingIconButton.value<IconButton *>();
    }
    auto *const iconButton = new IconButton;
    iconButton->assignDataFromAction(action);
    action->setProperty(propertyName.data(), QVariant::fromValue(iconButton));
    connect(action, &QAction::changed, iconButton, &IconButton::assignDataFromActionChangedSignal);
    connect(iconButton, &IconButton::clicked, action, &QAction::trigger);
    return iconButton;
}

/*!
 * \brief Internally called to assign data from a QAction to the icon button.
 */
void IconButton::assignDataFromActionChangedSignal()
{
    assignDataFromAction(qobject_cast<const QAction *>(QObject::sender()));
}

/*!
 * \brief Internally called to assign data from a QAction to the icon button.
 */
void IconButton::assignDataFromAction(const QAction *action)
{
    auto const icon = action->icon();
    const auto sizes = icon.availableSizes();
    const auto text = action->text();
    setPixmap(icon.pixmap(sizes.empty() ? defaultPixmapSize : sizes.front()));
    setToolTip(text.isEmpty() ? action->toolTip() : text);
}

QSize IconButton::sizeHint() const
{
#if QT_VERSION >= 0x050100
    const qreal pixmapRatio = m_pixmap.devicePixelRatio();
#else
    const qreal pixmapRatio = 1.0;
#endif
    return QSize(m_pixmap.width() / pixmapRatio, m_pixmap.height() / pixmapRatio);
}

void IconButton::paintEvent(QPaintEvent *)
{
#if QT_VERSION >= 0x050100
    const qreal pixmapRatio = m_pixmap.devicePixelRatio();
#else
    const qreal pixmapRatio = 1.0;
#endif
    QStylePainter painter(this);
    QRect pixmapRect = QRect(0, 0, m_pixmap.width() / pixmapRatio, m_pixmap.height() / pixmapRatio);
    pixmapRect.moveCenter(rect().center());
    painter.drawPixmap(pixmapRect, m_pixmap);
    if (hasFocus()) {
        QStyleOptionFocusRect focusOption;
        focusOption.initFrom(this);
        focusOption.rect = pixmapRect;
#ifdef Q_OS_MAC
        focusOption.rect.adjust(-4, -4, 4, 4);
        painter.drawControl(QStyle::CE_FocusFrame, focusOption);
#else
        painter.drawPrimitive(QStyle::PE_FrameFocusRect, focusOption);
#endif
    }
}

void IconButton::keyPressEvent(QKeyEvent *event)
{
    QAbstractButton::keyPressEvent(event);
    if (!event->modifiers() && (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)) {
        click();
    }
    event->accept();
}

void IconButton::keyReleaseEvent(QKeyEvent *event)
{
    QAbstractButton::keyReleaseEvent(event);
    event->accept();
}

} // namespace QtUtilities
