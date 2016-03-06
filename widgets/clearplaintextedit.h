#ifndef WIDGETS_CLEARPLAINTEXTEDIT_H
#define WIDGETS_CLEARPLAINTEXTEDIT_H

#include "./buttonoverlay.h"

#include <QPlainTextEdit>

namespace Widgets {

class LIB_EXPORT ClearPlainTextEdit : public QPlainTextEdit, public ButtonOverlay
{
    Q_OBJECT
public:
    explicit ClearPlainTextEdit(QWidget *parent = nullptr);
    ~ClearPlainTextEdit();
    bool isCleared() const;

private Q_SLOTS:
    void handleTextChanged();
    void handleClearButtonClicked();
    void handleScroll();

};

} // namespace Widgets

#endif // WIDGETS_CLEARPLAINTEXTEDIT_H
