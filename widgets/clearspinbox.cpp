#include "./clearspinbox.h"

#include <QHBoxLayout>
#include <QStyle>
#include <QStyleOptionSpinBox>

namespace QtUtilities {

/*!
 * \class ClearSpinBox
 * \brief A QSpinBox with an embedded button for clearing its contents and the
 * ability to hide
 *        the minimum value.
 */

/*!
 * \brief Constructs a clear spin box.
 */
ClearSpinBox::ClearSpinBox(QWidget *parent)
    : QSpinBox(parent)
    , ButtonOverlay(this, lineEdit())
    , m_minimumHidden(false)
{
    ButtonOverlay::setClearButtonEnabled(true);
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

void ClearSpinBox::handleCustomLayoutCreated()
{
    const QStyle *const s = style();
    QStyleOptionSpinBox opt;
    opt.initFrom(this);
    setContentsMarginsFromEditFieldRectAndFrameWidth(
        s->subControlRect(QStyle::CC_SpinBox, &opt, QStyle::SC_SpinBoxEditField, this), s->pixelMetric(QStyle::PM_SpinBoxFrameWidth, &opt, this));
    connect(this, static_cast<void (ClearSpinBox::*)(int)>(&ClearSpinBox::valueChanged), this, &ClearSpinBox::handleValueChanged);
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
} // namespace QtUtilities
