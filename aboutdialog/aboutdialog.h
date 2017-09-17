#ifndef DIALOGS_ABOUTDIALOG_H
#define DIALOGS_ABOUTDIALOG_H

#include "../global.h"

#include <QDialog>

#include <memory>

QT_FORWARD_DECLARE_CLASS(QGraphicsScene)

namespace Dialogs {

namespace Ui {
class AboutDialog;
}

class QT_UTILITIES_EXPORT AboutDialog : public QDialog {
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent, const QString &applicationName, const QString &creator, const QString &version,
        const QString &website = QString(), const QString &description = QString(), const QImage &image = QImage());
    explicit AboutDialog(QWidget *parent, const QString &description = QString(), const QImage &image = QImage());
    ~AboutDialog();

private:
    std::unique_ptr<Ui::AboutDialog> m_ui;
    QGraphicsScene *m_iconScene;
};
} // namespace Dialogs

#endif // DIALOGS_ABOUTDIALOG_H
