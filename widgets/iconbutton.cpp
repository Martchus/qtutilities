#include "./iconbutton.h"

#include <QStylePainter>
#include <QStyleOptionFocusRect>
#include <QKeyEvent>

namespace Widgets {

/*!
 * \class Widgets::IconButton
 * \brief A simple QAbstractButton implementation displaying a QPixmap.
 */

/*!
 * \brief Constructs an icon button.
 */
IconButton::IconButton(QWidget *parent) :
    QAbstractButton(parent)
{
    setCursor(Qt::ArrowCursor);
    setFocusPolicy(Qt::NoFocus);
}

/*!
 * \brief Destroys the icon button.
 */
IconButton::~IconButton()
{}

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
    if(hasFocus()) {
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

}
