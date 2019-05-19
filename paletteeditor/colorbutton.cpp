#include "./colorbutton.h"

#include <QApplication>
#include <QColorDialog>
#include <QDrag>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QPainter>

namespace Widgets {

/*!
 * \cond
 */

class ColorButtonPrivate {
    ColorButton *q_ptr;
    Q_DECLARE_PUBLIC(ColorButton)
public:
    QColor m_color;
#ifndef QT_NO_DRAGANDDROP
    QColor m_dragColor;
    QPoint m_dragStart;
    bool m_dragging;
#endif
    bool m_backgroundCheckered;

    void slotEditColor();
    QColor shownColor() const;
    QPixmap generatePixmap() const;
};

void ColorButtonPrivate::slotEditColor()
{
    const QColor newColor = QColorDialog::getColor(m_color, q_ptr, QString(), QColorDialog::ShowAlphaChannel);
    if (!newColor.isValid() || newColor == q_ptr->color())
        return;
    q_ptr->setColor(newColor);
}

QColor ColorButtonPrivate::shownColor() const
{
#ifndef QT_NO_DRAGANDDROP
    if (m_dragging)
        return m_dragColor;
#endif
    return m_color;
}

QPixmap ColorButtonPrivate::generatePixmap() const
{
    QPixmap pix(24, 24);

    int pixSize = 20;
    QBrush br(shownColor());

    QPixmap pm(2 * pixSize, 2 * pixSize);
    QPainter pmp(&pm);
    pmp.fillRect(0, 0, pixSize, pixSize, Qt::lightGray);
    pmp.fillRect(pixSize, pixSize, pixSize, pixSize, Qt::lightGray);
    pmp.fillRect(0, pixSize, pixSize, pixSize, Qt::darkGray);
    pmp.fillRect(pixSize, 0, pixSize, pixSize, Qt::darkGray);
    pmp.fillRect(0, 0, 2 * pixSize, 2 * pixSize, shownColor());
    br = QBrush(pm);

    QPainter p(&pix);
    int corr = 1;
    QRect r = pix.rect().adjusted(corr, corr, -corr, -corr);
    p.setBrushOrigin((r.width() % pixSize + pixSize) / 2 + corr, (r.height() % pixSize + pixSize) / 2 + corr);
    p.fillRect(r, br);

    p.fillRect(r.width() / 4 + corr, r.height() / 4 + corr, r.width() / 2, r.height() / 2, QColor(shownColor().rgb()));
    p.drawRect(pix.rect().adjusted(0, 0, -1, -1));

    return pix;
}

/*!
 * \endcond
 */

ColorButton::ColorButton(QWidget *parent)
    : QToolButton(parent)
    , d_ptr(new ColorButtonPrivate)
{
    d_ptr->q_ptr = this;
    d_ptr->m_dragging = false;
    d_ptr->m_backgroundCheckered = true;

    setAcceptDrops(true);

    connect(this, SIGNAL(clicked()), this, SLOT(slotEditColor()));
    setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
}

ColorButton::~ColorButton()
{
}

void ColorButton::setColor(const QColor &color)
{
    if (d_ptr->m_color == color)
        return;
    update();
    emit colorChanged(d_ptr->m_color = color);
}

QColor ColorButton::color() const
{
    return d_ptr->m_color;
}

void ColorButton::setBackgroundCheckered(bool checkered)
{
    if (d_ptr->m_backgroundCheckered == checkered)
        return;
    d_ptr->m_backgroundCheckered = checkered;
    update();
}

bool ColorButton::isBackgroundCheckered() const
{
    return d_ptr->m_backgroundCheckered;
}

void ColorButton::paintEvent(QPaintEvent *event)
{
    QToolButton::paintEvent(event);
    if (!isEnabled())
        return;

    const int pixSize = 10;
    QBrush br(d_ptr->shownColor());
    if (d_ptr->m_backgroundCheckered) {
        QPixmap pm(2 * pixSize, 2 * pixSize);
        QPainter pmp(&pm);
        pmp.fillRect(0, 0, pixSize, pixSize, Qt::white);
        pmp.fillRect(pixSize, pixSize, pixSize, pixSize, Qt::white);
        pmp.fillRect(0, pixSize, pixSize, pixSize, Qt::black);
        pmp.fillRect(pixSize, 0, pixSize, pixSize, Qt::black);
        pmp.fillRect(0, 0, 2 * pixSize, 2 * pixSize, d_ptr->shownColor());
        br = QBrush(pm);
    }

    QPainter p(this);
    const int corr = 4;
    QRect r = rect().adjusted(corr, corr, -corr, -corr);
    p.setBrushOrigin((r.width() % pixSize + pixSize) / 2 + corr, (r.height() % pixSize + pixSize) / 2 + corr);
    p.fillRect(r, br);

    // const int adjX = qRound(r.width() / 4.0);
    // const int adjY = qRound(r.height() / 4.0);
    // p.fillRect(r.adjusted(adjX, adjY, -adjX, -adjY),
    //           QColor(d_ptr->shownColor().rgb()));
    /*
  p.fillRect(r.adjusted(0, r.height() * 3 / 4, 0, 0),
             QColor(d_ptr->shownColor().rgb()));
  p.fillRect(r.adjusted(0, 0, 0, -r.height() * 3 / 4),
             QColor(d_ptr->shownColor().rgb()));
             */
    /*
  const QColor frameColor0(0, 0, 0, qRound(0.2 * (0xFF -
  d_ptr->shownColor().alpha())));
  p.setPen(frameColor0);
  p.drawRect(r.adjusted(adjX, adjY, -adjX - 1, -adjY - 1));
  */

    const QColor frameColor1(0, 0, 0, 26);
    p.setPen(frameColor1);
    p.drawRect(r.adjusted(1, 1, -2, -2));
    const QColor frameColor2(0, 0, 0, 51);
    p.setPen(frameColor2);
    p.drawRect(r.adjusted(0, 0, -1, -1));
}

void ColorButton::mousePressEvent(QMouseEvent *event)
{
#ifndef QT_NO_DRAGANDDROP
    if (event->button() == Qt::LeftButton)
        d_ptr->m_dragStart = event->pos();
#endif
    QToolButton::mousePressEvent(event);
}

void ColorButton::mouseMoveEvent(QMouseEvent *event)
{
#ifndef QT_NO_DRAGANDDROP
    if (event->buttons() & Qt::LeftButton && (d_ptr->m_dragStart - event->pos()).manhattanLength() > QApplication::startDragDistance()) {
        QMimeData *mime = new QMimeData;
        mime->setColorData(color());
        QDrag *drg = new QDrag(this);
        drg->setMimeData(mime);
        drg->setPixmap(d_ptr->generatePixmap());
        setDown(false);
        event->accept();
        drg->start();
        return;
    }
#endif
    QToolButton::mouseMoveEvent(event);
}

#ifndef QT_NO_DRAGANDDROP
void ColorButton::dragEnterEvent(QDragEnterEvent *event)
{
    const QMimeData *mime = event->mimeData();
    if (!mime->hasColor())
        return;

    event->accept();
    d_ptr->m_dragColor = qvariant_cast<QColor>(mime->colorData());
    d_ptr->m_dragging = true;
    update();
}

void ColorButton::dragLeaveEvent(QDragLeaveEvent *event)
{
    event->accept();
    d_ptr->m_dragging = false;
    update();
}

void ColorButton::dropEvent(QDropEvent *event)
{
    event->accept();
    d_ptr->m_dragging = false;
    if (d_ptr->m_dragColor == color())
        return;
    setColor(d_ptr->m_dragColor);
}
#endif
} // namespace Widgets

#include "moc_colorbutton.cpp"
