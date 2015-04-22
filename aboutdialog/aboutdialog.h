#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <c++utilities/application/global.h>

#include <QDialog>

#include <memory>

QT_BEGIN_NAMESPACE
class QGraphicsScene;
QT_END_NAMESPACE

namespace Dialogs
{

namespace Ui {
class AboutDialog;
}

class LIB_EXPORT AboutDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit AboutDialog(QWidget *parent, const QString &applicationName, const QString &creator, const QString &version, const QString &website = QString(), const QString &description = QString(), const QImage &image = QImage());
    explicit AboutDialog(QWidget *parent, const QString &description = QString(), const QImage &image = QImage());
    ~AboutDialog();

private:
    std::unique_ptr<Ui::AboutDialog> m_ui;
    QGraphicsScene *m_iconScene;
};

}


#endif // ABOUTDIALOG_H
