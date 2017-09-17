#include "./clearspinbox.h"

#include <QHBoxLayout>
#include <QStyle>
#include <QStyleOptionSpinBox>

namespace Widgets {

/*!
 * \class Widgets::ClearSpinBox
 * \brief A QSpinBox with an embedded button for clearing its contents and the
 * ability to hide
 *        the minimum value.
 */

/*!
 * \brief Constructs a clear spin box.
 */
ClearSpinBox::ClearSpinBox(QWidget *parent)
    : QSpinBox(parent)
    , ButtonOverlay(this)
    , m_minimumHidden(false)
{
    const QMargins margins = contentsMargins();
    QStyleOptionComboBox opt;
    opt.initFrom(this);
    const int frameWidth = style()->pixelMetric(QStyle::PM_SpinBoxFrameWidth, &opt, this);
    const int pad = 5;
    const int buttonWidth = style()->subControlRect(QStyle::CC_SpinBox, &opt, QStyle::SC_SpinBoxUp, this).width() + 10;
    buttonLayout()->setContentsMargins(margins.left() + frameWidth + pad, margins.top() + frameWidth,
        margins.right() + frameWidth + pad + buttonWidth, margins.bottom() + frameWidth);
    setClearButtonEnabled(true);
    connect(this, static_cast<void (ClearSpinBox::*)(int)>(&ClearSpinBox::valueChanged), this, &ClearSpinBox::handleValueChanged);
}

/*!
 * \brief Destroys the clear spin box.
 */
ClearSpinBox::~ClearSpinBox()
{
}

/*!
 * \brief Updates the visibility of the clear button.
 */
void ClearSpinBox::handleValueChanged(int value)
{
    updateClearButtonVisibility(value != minimum());
}

void ClearSpinBox::handleClearButtonClicked()
{
    setValue(minimum());
}

bool ClearSpinBox::isCleared() const
{
    return value() == minimum();
}

int ClearSpinBox::valueFromText(const QString &text) const
{
    if (m_minimumHidden && text.isEmpty()) {
        return minimum();
    } else {
        return QSpinBox::valueFromText(text);
    }
}

QString ClearSpinBox::textFromValue(int val) const
{
    if (m_minimumHidden && (val == minimum())) {
        return QString();
    } else {
        return QSpinBox::textFromValue(val);
    }
}
} // namespace Widgets
