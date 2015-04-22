#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include <QGraphicsPixmapItem>
#include <QApplication>
#include <QDesktopWidget>
#include <QStyle>
#include <QMessageBox>

/*!
    \namespace Dialogs
    \brief Provides common dialogs such as AboutDialog, EnterPasswordDialog and SettingsDialog.
*/

namespace Dialogs {

/*!
 * \class Dialogs::AboutDialog
 * \brief The AboutDialog class provides a simple about dialog.
 */

/*!
 * \brief Constructs an about dialog with the provided information.
 * \param parent Specifies the parent widget.
 * \param applicationName Specifies the name of the application. If empty, QApplication::applicationName() will be used.
 * \param creator Specifies the creator of the application. If empty, QApplication::organizationName() will be used.
 * \param version Specifies the version of the application. If empty, QApplication::applicationVersion() will be used.
 * \param description Specifies a short description about the application.
 * \param website Specifies the URL to the website of the application. If empty, QApplication::organizationDomain() will be used.
 * \param image Specifies the application icon. If the image is null, the standard information icon will be used.
 */
AboutDialog::AboutDialog(QWidget *parent, const QString &applicationName, const QString &creator, const QString &version, const QString &website, const QString &description, const QImage &image) :
    QDialog(parent),
    m_ui(new Ui::AboutDialog)
{
    m_ui->setupUi(this);
#ifdef Q_OS_WIN32
    setStyleSheet(QStringLiteral("* { font: 9pt \"Segoe UI\"; } #mainWidget { color: black; background-color: white; border: none; } #productNameLabel { font-size: 12pt; color: #003399; }"));
#else
    setStyleSheet(QStringLiteral("#productNameLabel { font-weight: bold; }"));
#endif
    setWindowFlags(Qt::Tool);
    if(!applicationName.isEmpty()) {
        m_ui->productNameLabel->setText(applicationName);
    } else if(!QApplication::applicationDisplayName().isEmpty()) {
        m_ui->productNameLabel->setText(QApplication::applicationDisplayName());
    } else {
        m_ui->productNameLabel->setText(QApplication::applicationName());
    }
    if(!creator.isEmpty()) {
        m_ui->creatorLabel->setText(creator);
    } else {
        m_ui->creatorLabel->setText(QApplication::organizationName());
    }
    if(!version.isEmpty()) {
        m_ui->versionLabel->setText(version);
    } else {
        m_ui->versionLabel->setText(QApplication::applicationVersion());
    }
    m_ui->descLabel->setText(description);
    if(!website.isEmpty()) {
        m_ui->websiteLabel->setText(tr("<a href=\"%1\">Website</a>").arg(website));
    } else {
        m_ui->websiteLabel->setText(tr("<a href=\"%1\">Website</a>").arg(QApplication::organizationDomain()));
    }
    m_iconScene = new QGraphicsScene(this);
    if(!image.isNull()) {
        m_iconScene->addItem(new QGraphicsPixmapItem(QPixmap::fromImage(image)));
    } else {
        m_iconScene->addItem(new QGraphicsPixmapItem(QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation, nullptr, this).pixmap(128)));
    }
    m_ui->graphicsView->setScene(m_iconScene);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), parentWidget() ? parentWidget()->geometry() : QApplication::desktop()->availableGeometry()));
}

/*!
 * \brief Constructs an about dialog with the specified \a parent, \a description and \a image.
 */
AboutDialog::AboutDialog(QWidget *parent, const QString &description, const QImage &image) :
    AboutDialog(parent, QString(), QString(), QString(), QString(), description, image)
{}

/*!
 * \brief Destroys the about dialog.
 */
AboutDialog::~AboutDialog()
{}

}
