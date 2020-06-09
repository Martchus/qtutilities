#ifndef WIDGETS_BUTTONOVERLAY_H
#define WIDGETS_BUTTONOVERLAY_H

#include "../global.h"

#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QWidget)
QT_FORWARD_DECLARE_CLASS(QHBoxLayout)
QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QPixmap)
QT_FORWARD_DECLARE_CLASS(QMargins)
QT_FORWARD_DECLARE_CLASS(QRect)
QT_FORWARD_DECLARE_CLASS(QLineEdit)

namespace QtUtilities {

class IconButton;
class ClearComboBox;
class ClearSpinBox;
class ClearPlainTextEdit;
class ClearLineEdit;

class QT_UTILITIES_EXPORT ButtonOverlay {
    // allow these derived classes to use private helpers provided by ButtonOverlay
    friend class ClearComboBox;
    friend class ClearSpinBox;
    friend class ClearPlainTextEdit;
    friend class ClearLineEdit;

public:
    explicit ButtonOverlay(QWidget *widget);
    explicit ButtonOverlay(QWidget *widget, QLineEdit *lineEdit);
    virtual ~ButtonOverlay();

    bool isUsingCustomLayout() const;
    QHBoxLayout *buttonLayout();
    bool isClearButtonEnabled() const;
    void setClearButtonEnabled(bool enabled);
    bool isInfoButtonEnabled() const;
    void enableInfoButton(const QPixmap &pixmap, const QString &infoText);
    void disableInfoButton();
    void addCustomButton(QWidget *button);
    void insertCustomButton(int index, QWidget *button);
    void removeCustomButton(QWidget *button);
    void addCustomAction(QAction *action);
    void insertCustomAction(int index, QAction *action);
    void removeCustomAction(QAction *action);
    virtual bool isCleared() const;

protected:
    void updateClearButtonVisibility(bool visible);
    virtual void handleClearButtonClicked();
    virtual void handleCustomLayoutCreated();

private:
    void fallbackToUsingCustomLayout();
    QLineEdit *lineEditForWidget() const;
    void showInfo();
    void setContentsMarginsFromEditFieldRectAndFrameWidth(const QRect &editFieldRect, int frameWidth, int padding = 0);

    QWidget *m_widget;
    QWidget *m_buttonWidget;
    QHBoxLayout *m_buttonLayout;
    IconButton *m_clearButton;
    void *m_infoButtonOrAction;
};

} // namespace QtUtilities

#endif // WIDGETS_BUTTONOVERLAY_H
