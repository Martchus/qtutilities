#ifndef WIDGETS_BUTTONOVERLAY_H
#define WIDGETS_BUTTONOVERLAY_H

#include "../global.h"

#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QWidget)
QT_FORWARD_DECLARE_CLASS(QHBoxLayout)
QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QPixmap)

namespace QtUtilities {

class IconButton;

class QT_UTILITIES_EXPORT ButtonOverlay {
public:
    explicit ButtonOverlay(QWidget *widget);
    virtual ~ButtonOverlay();

    QHBoxLayout *buttonLayout();
    bool isClearButtonEnabled() const;
    void setClearButtonEnabled(bool enabled);
    bool isInfoButtonEnabled() const;
    void enableInfoButton(const QPixmap &pixmap, const QString &infoText);
    void disableInfoButton();
    void addCustomButton(QWidget *button);
    void insertCustomButton(int index, QWidget *button);
    void removeCustomButton(QWidget *button);
    virtual bool isCleared() const;

protected:
    void updateClearButtonVisibility(bool visible);
    virtual void handleClearButtonClicked();

private:
    void showInfo();

    QWidget *m_widget;
    QWidget *m_buttonWidget;
    QHBoxLayout *m_buttonLayout;
    IconButton *m_clearButton;
    IconButton *m_infoButton;
};

/*!
 * \brief Returns the layout manager holding the buttons.
 */
inline QHBoxLayout *ButtonOverlay::buttonLayout()
{
    return m_buttonLayout;
}

/*!
 * \brief Returns whether the clear button is enabled.
 */
inline bool ButtonOverlay::isClearButtonEnabled() const
{
    return m_clearButton != nullptr;
}

/*!
 * \brief Returns whether the info button is enabled.
 */
inline bool ButtonOverlay::isInfoButtonEnabled() const
{
    return m_infoButton != nullptr;
}
} // namespace QtUtilities

#endif // WIDGETS_BUTTONOVERLAY_H
