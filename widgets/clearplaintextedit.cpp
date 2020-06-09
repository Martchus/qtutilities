#include "./clearplaintextedit.h"

#include <QHBoxLayout>
#include <QScrollBar>
#include <QStyle>
#include <QStyleOptionFrame>

using namespace std;

namespace QtUtilities {

/*!
 * \class ClearPlainTextEdit
 * \brief A QPlainTextEdit with an embedded button for clearing its contents.
 */

/*!
 * \brief Constructs a clear plain text edit.
 */
ClearPlainTextEdit::ClearPlainTextEdit(QWidget *parent)
    : QPlainTextEdit(parent)
    , ButtonOverlay(viewport())
{
    handleCustomLayoutCreated();
    ButtonOverlay::setClearButtonEnabled(true);
}

/*!
 * \brief Destroys the clear plain text edit.
 */
ClearPlainTextEdit::~ClearPlainTextEdit()
{
}

/*!
 * \brief Updates the visibility of the clear button.
 */
void ClearPlainTextEdit::handleTextChanged()
{
    updateClearButtonVisibility(!document()->isEmpty());
}

void ClearPlainTextEdit::handleClearButtonClicked()
{
    // do no call clear() here to prevent clearing of undo history
    QTextCursor cursor(document());
    cursor.select(QTextCursor::Document);
    cursor.removeSelectedText();
}

void ClearPlainTextEdit::handleCustomLayoutCreated()
{
    // set alignment to show buttons in the bottom right corner
    ButtonOverlay::buttonLayout()->setAlignment(Qt::AlignBottom | Qt::AlignRight);
    const QStyle *const s = style();
    QStyleOptionFrame opt;
    opt.initFrom(this);
    setContentsMarginsFromEditFieldRectAndFrameWidth(s->subElementRect(QStyle::SE_FrameContents, &opt, this),
        s->pixelMetric(QStyle::PM_DefaultFrameWidth, &opt, m_widget), s->pixelMetric(QStyle::PM_LayoutVerticalSpacing, &opt, m_widget));
    connect(this, &QPlainTextEdit::textChanged, this, &ClearPlainTextEdit::handleTextChanged);
    // ensure button layout is realigned when scrolling
    connect(verticalScrollBar(), &QScrollBar::actionTriggered, this, &ClearPlainTextEdit::handleScroll);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &ClearPlainTextEdit::handleScroll);
}

void ClearPlainTextEdit::handleScroll()
{
    buttonLayout()->update();
}

bool ClearPlainTextEdit::isCleared() const
{
    return document()->isEmpty();
}

} // namespace QtUtilities
