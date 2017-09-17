#ifndef WIDGETS_CLEARLINEEDIT_H
#define WIDGETS_CLEARLINEEDIT_H

#include "./buttonoverlay.h"

#include <QLineEdit>

QT_FORWARD_DECLARE_CLASS(QHBoxLayout)

namespace Widgets {

class IconButton;

class QT_UTILITIES_EXPORT ClearLineEdit : public QLineEdit, public ButtonOverlay {
    Q_OBJECT
public:
    explicit ClearLineEdit(QWidget *parent = nullptr);
    ~ClearLineEdit();
    bool isCleared() const;

private Q_SLOTS:
    void handleTextChanged(const QString &text);
    void handleClearButtonClicked();
};
} // namespace Widgets

#endif // WIDGETS_CLEARLINEEDIT_H
