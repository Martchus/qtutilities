#ifndef WIDGETS_CLEARSPINBOX_H
#define WIDGETS_CLEARSPINBOX_H

#include "buttonoverlay.h"

#include <c++utilities/application/global.h>

#include <QSpinBox>
#include <QLineEdit>

QT_BEGIN_NAMESPACE
class QHBoxLayout;
QT_END_NAMESPACE

namespace Widgets {

class IconButton;

class LIB_EXPORT ClearSpinBox : public QSpinBox, public ButtonOverlay
{
    Q_OBJECT
    Q_PROPERTY(bool minimumHidden READ minimumHidden WRITE setMinimumHidden)
    Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText)
    Q_PROPERTY(bool isCleared READ isCleared)

public:
    explicit ClearSpinBox(QWidget *parent = nullptr);
    ~ClearSpinBox();
    bool minimumHidden() const;
    void setMinimumHidden(bool value);
    QString placeholderText() const;
    void setPlaceholderText(const QString &placeholderText);
    bool isCleared() const;

protected:
    int valueFromText(const QString &text) const;
    QString textFromValue(int val) const;

private Q_SLOTS:
    void handleValueChanged(int value);
    void handleClearButtonClicked();

private:
    bool m_minimumHidden;

};

/*!
 * \brief Returns whether the minimum value will be hidden.
 */
inline bool ClearSpinBox::minimumHidden() const
{
    return m_minimumHidden;
}

/*!
 * \brief Sets whether the minimum value should be hidden.
 */
inline void ClearSpinBox::setMinimumHidden(bool value)
{
    m_minimumHidden = value;
}

/*!
 * \brief Returns the placeholder text.
 * \sa QLineEdit::placeholderText()
 */
inline QString ClearSpinBox::placeholderText() const
{
    return lineEdit()->placeholderText();
}

/*!
 * \brief Sets the placeholder text.
 * \sa QLineEdit::setPlaceholderText()
 */
inline void ClearSpinBox::setPlaceholderText(const QString &placeholderText)
{
    lineEdit()->setPlaceholderText(placeholderText);
}

}

#endif // WIDGETS_CLEARSPINBOX_H
