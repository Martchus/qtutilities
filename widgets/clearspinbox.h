#ifndef WIDGETS_CLEARSPINBOX_H
#define WIDGETS_CLEARSPINBOX_H

#include "./buttonoverlay.h"

#include <QLineEdit>
#include <QSpinBox>

QT_FORWARD_DECLARE_CLASS(QHBoxLayout)

namespace QtUtilities {

class IconButton;

class QT_UTILITIES_EXPORT ClearSpinBox : public QSpinBox, public ButtonOverlay {
    Q_OBJECT
    Q_PROPERTY(bool cleared READ isCleared)
    Q_PROPERTY(bool minimumHidden READ minimumHidden WRITE setMinimumHidden)
    Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText)

public:
    explicit ClearSpinBox(QWidget *parent = nullptr);
    ~ClearSpinBox() override;
    bool minimumHidden() const;
    void setMinimumHidden(bool value);
    QString placeholderText() const;
    void setPlaceholderText(const QString &placeholderText);
    bool isCleared() const override;

protected:
    int valueFromText(const QString &text) const override;
    QString textFromValue(int val) const override;

private Q_SLOTS:
    void handleValueChanged(int value);
    void handleClearButtonClicked() override;
    void handleCustomLayoutCreated() override;

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
} // namespace QtUtilities

#endif // WIDGETS_CLEARSPINBOX_H
