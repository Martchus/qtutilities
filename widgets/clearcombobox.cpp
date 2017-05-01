#include "./clearcombobox.h"

#include <QHBoxLayout>
#include <QStyle>
#include <QStyleOptionComboBox>

namespace Widgets {

/*!
 * \class Widgets::ClearComboBox
 * \brief A QComboBox with an embedded button for clearing its contents.
 */

/*!
 * \brief Constructs a clear combo box.
 */
ClearComboBox::ClearComboBox(QWidget *parent)
    : QComboBox(parent)
    , ButtonOverlay(this)
{
    const QMargins margins = contentsMargins();
    QStyleOptionComboBox opt;
    opt.initFrom(this);
    const int frameWidth = style()->pixelMetric(QStyle::PM_ComboBoxFrameWidth, &opt, this);
    const int pad = 2;
    const int buttonWidth = style()->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxArrow, this).width();
    buttonLayout()->setContentsMargins(margins.left() + frameWidth + pad, margins.top() + frameWidth,
        margins.right() + frameWidth + pad + buttonWidth, margins.bottom() + frameWidth);
    setClearButtonEnabled(isEditable());
    connect(this, &ClearComboBox::currentTextChanged, this, &ClearComboBox::handleTextChanged);
}

/*!
 * \brief Destroys the clear combo box.
 */
ClearComboBox::~ClearComboBox()
{
}

/*!
 * \brief Updates the visibility of the clear button.
 */
void ClearComboBox::handleTextChanged(const QString &text)
{
    updateClearButtonVisibility(!text.isEmpty());
}

void ClearComboBox::handleClearButtonClicked()
{
    clearEditText();
}

bool ClearComboBox::isCleared() const
{
    return currentText().isEmpty();
}
}
