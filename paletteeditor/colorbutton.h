#ifndef WIDGETS_COLORBUTTON_H
#define WIDGETS_COLORBUTTON_H

#include "../global.h"

#include <QToolButton>

namespace QtUtilities {

/*!
 * \brief The ColorButton class is used by PaletteEditor.
 *
 * This is taken from qttools/src/shared/qtgradienteditor/qtcolorbutton.h.
 */
class QT_UTILITIES_EXPORT ColorButton : public QToolButton {
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(bool backgroundCheckered READ isBackgroundCheckered WRITE setBackgroundCheckered)
public:
    ColorButton(QWidget *parent = nullptr);
    ~ColorButton() override;

    bool isBackgroundCheckered() const;
    void setBackgroundCheckered(bool checkered);

    QColor color() const;

public Q_SLOTS:
    void setColor(const QColor &color);

Q_SIGNALS:
    void colorChanged(const QColor &color);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
#ifndef QT_NO_DRAGANDDROP
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
#endif

private:
    QScopedPointer<class ColorButtonPrivate> d_ptr;
    Q_DECLARE_PRIVATE(ColorButton)
    Q_DISABLE_COPY(ColorButton)
    Q_PRIVATE_SLOT(d_func(), void slotEditColor())
};
} // namespace QtUtilities

#endif // WIDGETS_COLORBUTTON_H
