#include "./clearcombobox.h"

#include <QHBoxLayout>
#include <QStyle>
#include <QStyleOptionComboBox>

namespace QtUtilities {

/*!
 * \class ClearComboBox
 * \brief A QComboBox with an embedded button for clearing its contents.
 */

/// \cond
static inline auto *getComboBoxLineEdit(QComboBox *comboBox)
{
    comboBox->setEditable(true);
    return comboBox->lineEdit();
}
/// \endcond

/*!
 * \brief Constructs a clear combo box.
 * \remarks The combo box is initialized to be editable and which must not be changed.
 */
ClearComboBox::ClearComboBox(QWidget *parent)
    : QComboBox(parent)
    , ButtonOverlay(this, getComboBoxLineEdit(this))
{
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

void ClearComboBox::handleCustomLayoutCreated()
{
    const QStyle *const s = style();
    QStyleOptionComboBox opt;
    opt.initFrom(this);
    setContentsMarginsFromEditFieldRectAndFrameWidth(
        s->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxEditField, this), s->pixelMetric(QStyle::PM_ComboBoxFrameWidth, &opt, this));
    connect(this, &ClearComboBox::currentTextChanged, this, &ClearComboBox::handleTextChanged);
}

bool ClearComboBox::isCleared() const
{
    return currentText().isEmpty();
}
} // namespace QtUtilities
