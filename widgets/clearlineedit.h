#ifndef WIDGETS_TAGFIELDLINEEDIT_H
#define WIDGETS_TAGFIELDLINEEDIT_H

#include "./buttonoverlay.h"

#include <c++utilities/application/global.h>

#include <QLineEdit>

QT_BEGIN_NAMESPACE
class QHBoxLayout;
QT_END_NAMESPACE

namespace Widgets {

class IconButton;

class LIB_EXPORT ClearLineEdit : public QLineEdit, public ButtonOverlay
{
    Q_OBJECT
public:
    explicit ClearLineEdit(QWidget *parent = nullptr);
    ~ClearLineEdit();
    bool isCleared() const;
    
private Q_SLOTS:
    void handleTextChanged(const QString &text);
    void handleClearButtonClicked();
};

}

#endif // WIDGETS_TAGFIELDLINEEDIT_H
