#include "./clearlineedit.h"

#include <QStyle>
#include <QStyleOptionFrame>

namespace QtUtilities {

/*!
 * \class ClearLineEdit
 * \brief A QLineEdit with an embedded button for clearing its contents.
 */

/*!
 * \brief Constructs a clear line edit.
 */
ClearLineEdit::ClearLineEdit(QWidget *parent)
    : QLineEdit(parent)
    , ButtonOverlay(this)
{
    const QStyle *const s = style();
    QStyleOptionFrame opt;
    opt.initFrom(this);
    setContentsMarginsFromEditFieldRectAndFrameWidth(s->subElementRect(QStyle::SE_LineEditContents, &opt, this),
        s->pixelMetric(QStyle::PM_DefaultFrameWidth, &opt, m_widget), s->pixelMetric(QStyle::PM_LayoutVerticalSpacing, &opt, m_widget));
    ButtonOverlay::setClearButtonEnabled(true);
    connect(this, &ClearLineEdit::textChanged, this, &ClearLineEdit::handleTextChanged);
}

/*!
 * \brief Destroys the clear combo box.
 */
ClearLineEdit::~ClearLineEdit()
{
}

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
} // namespace QtUtilities
