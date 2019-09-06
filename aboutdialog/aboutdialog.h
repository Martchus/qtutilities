#ifndef DIALOGS_ABOUTDIALOG_H
#define DIALOGS_ABOUTDIALOG_H

#include "../global.h"

#include <QDialog>

#include <memory>

QT_FORWARD_DECLARE_CLASS(QGraphicsScene)

namespace QtUtilities {

namespace Ui {
class AboutDialog;
}

class QT_UTILITIES_EXPORT AboutDialog : public QDialog {
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent, const QString &applicationName, const QString &creator, const QString &version,
        const QString &website = QString(), const QString &description = QString(), const QImage &image = QImage());
    explicit AboutDialog(QWidget *parent, const QString &applicationName, const QString &creator, const QString &version,
        const std::vector<const char *> &dependencyVersions, const QString &website = QString(), const QString &description = QString(),
        const QImage &image = QImage());
    explicit AboutDialog(QWidget *parent, const QString &website = QString(), const QString &description = QString(), const QImage &image = QImage());
    ~AboutDialog() override;

private Q_SLOTS:
    void linkActivated(const QString &link);

private:
    std::unique_ptr<Ui::AboutDialog> m_ui;
    QGraphicsScene *m_iconScene;
};
} // namespace QtUtilities

#endif // DIALOGS_ABOUTDIALOG_H
