#ifndef WIDGETS_CLEARLINEEDIT_H
#define WIDGETS_CLEARLINEEDIT_H

#include "./buttonoverlay.h"

#include <QLineEdit>

QT_FORWARD_DECLARE_CLASS(QHBoxLayout)

namespace QtUtilities {

class IconButton;

class QT_UTILITIES_EXPORT ClearLineEdit : public QLineEdit, public ButtonOverlay {
    Q_OBJECT
    Q_PROPERTY(bool cleared READ isCleared)

public:
    explicit ClearLineEdit(QWidget *parent = nullptr);
    ~ClearLineEdit() override;
    bool isCleared() const override;

private Q_SLOTS:
    void handleTextChanged(const QString &text);
    void handleClearButtonClicked() override;
};
} // namespace QtUtilities

#endif // WIDGETS_CLEARLINEEDIT_H
