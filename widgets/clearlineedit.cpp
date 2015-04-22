#include "clearlineedit.h"

namespace Widgets {

/*!
 * \class Widgets::ClearLineEdit
 * \brief A QLineEdit with an embedded button for clearing its contents.
 */

/*!
 * \brief Constructs a clear line edit.
 */
ClearLineEdit::ClearLineEdit(QWidget *parent) :
    QLineEdit(parent),
    ButtonOverlay(this)
{
    ButtonOverlay::setClearButtonEnabled(true);
    connect(this, &ClearLineEdit::textChanged, this, &ClearLineEdit::handleTextChanged);
}

/*!
 * \brief Destroys the clear combo box.
 */
ClearLineEdit::~ClearLineEdit()
{}

/*!
 * \brief Updates the visibility of the clear button.
 */
void ClearLineEdit::handleTextChanged(const QString &text)
{
    updateClearButtonVisibility(!text.isEmpty());
}

void ClearLineEdit::handleClearButtonClicked()
{
    clear();
}

bool ClearLineEdit::isCleared() const
{
    return text().isEmpty();
}

}
