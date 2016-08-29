#ifndef WIDGETS_COLORBUTTON_H
#define WIDGETS_COLORBUTTON_H

#include "../global.h"

#include <QToolButton>

namespace Widgets {

/*!
 * \brief The ColorButton class is used by PaletteEditor.
 *
 * This is taken from qttools/src/shared/qtgradienteditor/qtcolorbutton.h.
 */
class QT_UTILITIES_EXPORT ColorButton : public QToolButton
{
    Q_OBJECT
    Q_PROPERTY(bool backgroundCheckered READ isBackgroundCheckered WRITE setBackgroundCheckered)
public:
    ColorButton(QWidget *parent = nullptr);
    ~ColorButton();

    bool isBackgroundCheckered() const;
    void setBackgroundCheckered(bool checkered);

    QColor color() const;

public Q_SLOTS:
    void setColor(const QColor &color);

Q_SIGNALS:
    void colorChanged(const QColor &color);

protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
#ifndef QT_NO_DRAGANDDROP
    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);
#endif

private:
    QScopedPointer<class ColorButtonPrivate> d_ptr;
    Q_DECLARE_PRIVATE(ColorButton)
    Q_DISABLE_COPY(ColorButton)
    Q_PRIVATE_SLOT(d_func(), void slotEditColor())
};

}

#endif // WIDGETS_COLORBUTTON_H
