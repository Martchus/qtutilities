#include "./clearcombobox.h"

#include <QHBoxLayout>
#include <QStyle>
#include <QStyleOptionComboBox>

namespace QtUtilities {

/*!
 * \class ClearComboBox
 * \brief A QComboBox with an embedded button for clearing its contents.
 */

/*!
 * \brief Constructs a clear combo box.
 */
ClearComboBox::ClearComboBox(QWidget *parent)
    : QComboBox(parent)
    , ButtonOverlay(this)
{
    const QStyle *const s = style();
    QStyleOptionComboBox opt;
    opt.initFrom(this);
    setContentsMarginsFromEditFieldRectAndFrameWidth(
        s->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxEditField, this), s->pixelMetric(QStyle::PM_ComboBoxFrameWidth, &opt, this));
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
} // namespace QtUtilities
