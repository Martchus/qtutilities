#ifndef WIDGETS_CLEARCOMBOBOX_H
#define WIDGETS_CLEARCOMBOBOX_H

#include "./buttonoverlay.h"

#include <QComboBox>

namespace Widgets {

class QT_UTILITIES_EXPORT ClearComboBox : public QComboBox, public ButtonOverlay
{
    Q_OBJECT
public:
    explicit ClearComboBox(QWidget *parent = nullptr);
    ~ClearComboBox();
    bool isCleared() const;
    
private Q_SLOTS:
    void handleTextChanged(const QString &text);
    void handleClearButtonClicked();
    
};

} // namespace Widgets

#endif // WIDGETS_CLEARCOMBOBOX_H
