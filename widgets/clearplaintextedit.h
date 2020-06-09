#ifndef WIDGETS_CLEARPLAINTEXTEDIT_H
#define WIDGETS_CLEARPLAINTEXTEDIT_H

#include "./buttonoverlay.h"

#include <QPlainTextEdit>

namespace QtUtilities {

class QT_UTILITIES_EXPORT ClearPlainTextEdit : public QPlainTextEdit, public ButtonOverlay {
    Q_OBJECT
    Q_PROPERTY(bool cleared READ isCleared)

public:
    explicit ClearPlainTextEdit(QWidget *parent = nullptr);
    ~ClearPlainTextEdit() override;
    bool isCleared() const override;

private Q_SLOTS:
    void handleTextChanged();
    void handleClearButtonClicked() override;
    void handleCustomLayoutCreated() override;
    void handleScroll();
};

} // namespace QtUtilities

#endif // WIDGETS_CLEARPLAINTEXTEDIT_H
