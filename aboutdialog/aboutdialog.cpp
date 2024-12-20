#include "./aboutdialog.h"
#include "../misc/dialogutils.h"

#include "ui_aboutdialog.h"

#include <c++utilities/application/argumentparser.h>

#include <QApplication>
#include <QGraphicsPixmapItem>
#include <QMessageBox>
#include <QStringBuilder>
#include <QStyle>

/*!
 * \brief The QtUtilities namespace contains all utilities provided by the qtutilities library.
 */
namespace QtUtilities {

/*!
 * \class AboutDialog
 * \brief The AboutDialog class provides a simple about dialog.
 */

/*!
 * \brief Constructs an about dialog with the provided information.
 * \param parent Specifies the parent widget.
 * \param applicationName Specifies the name of the application. If empty,
 * QApplication::applicationName() will be used.
 * \param creator Specifies the creator of the application. If empty,
 * QApplication::organizationName() will be used.
 * \param version Specifies the version of the application. If empty,
 * QApplication::applicationVersion() will be used.
 * \param dependencyVersions Specifies the dependency versions which were present at link-time. If empty,
 * the application info from c++utilities is used.
 * \param description Specifies a short description about the application.
 * \param website Specifies the URL to the website of the application.
 * \param image Specifies the application icon. If the image is null, the
 * standard information icon will be used.
 */
AboutDialog::AboutDialog(QWidget *parent, const QString &applicationName, const QString &creator, const QString &version,
    const std::vector<const char *> &dependencyVersions, const QString &website, const QString &description, const QImage &image)
    : QDialog(parent)
    , m_ui(new Ui::AboutDialog)
{
    m_ui->setupUi(this);
    makeHeading(m_ui->productNameLabel);
    setStyleSheet(dialogStyleForPalette(palette()));
    setWindowFlags((windowFlags()) & ~(Qt::WindowMinMaxButtonsHint | Qt::WindowContextHelpButtonHint | Qt::WindowFullscreenButtonHint));
    if (!applicationName.isEmpty()) {
        m_ui->productNameLabel->setText(applicationName);
    } else if (!QApplication::applicationDisplayName().isEmpty()) {
        m_ui->productNameLabel->setText(QApplication::applicationDisplayName());
    } else {
        m_ui->productNameLabel->setText(QApplication::applicationName());
    }
    if (creator.startsWith(QLatin1Char('<'))) {
        // assign rich text as-is
        m_ui->creatorLabel->setText(creator);
    } else {
        // add "developed by " before creator name
        auto setCreator = [this, creator = std::move(creator)] {
            m_ui->creatorLabel->setText(tr("developed by %1").arg(creator.isEmpty() ? QApplication::organizationName() : creator));
        };
        setCreator();
        connect(this, &AboutDialog::retranslationRequired, this, std::move(setCreator));
    }
    m_ui->versionLabel->setText(version.isEmpty() ? QApplication::applicationVersion() : version);
    const auto &deps(dependencyVersions.size() ? dependencyVersions : CppUtilities::applicationInfo.dependencyVersions);
    if (!deps.empty()) {
        QStringList linkedAgainst;
        linkedAgainst.reserve(static_cast<int>(deps.size()));
        for (const auto &dependencyVersion : deps) {
            linkedAgainst << QString::fromUtf8(dependencyVersion);
        }
        m_ui->versionLabel->setToolTip(QStringLiteral("<p>") % tr("Linked against:") % QStringLiteral("</p><ul><li>")
            % linkedAgainst.join(QStringLiteral("</li><li>")) % QStringLiteral("</li></ul>"));
    }
    if (!website.isEmpty() || CppUtilities::applicationInfo.url) {
        auto setWebsite = [this, website = std::move(website)] {
            m_ui->websiteLabel->setText(tr("For updates and bug reports visit the <a href=\"%1\" "
                                           "style=\"text-decoration: underline; color: palette(link);\">project "
                                           "website</a>.")
                    .arg(!website.isEmpty() ? website : QString::fromUtf8(CppUtilities::applicationInfo.url)));
        };
        setWebsite();
        connect(this, &AboutDialog::retranslationRequired, this, std::move(setWebsite));
    } else {
        m_ui->websiteLabel->hide();
    }
    m_ui->descLabel->setText(description.isEmpty() && CppUtilities::applicationInfo.description
            ? QString::fromUtf8(CppUtilities::applicationInfo.description)
            : description);
    m_iconScene = new QGraphicsScene(this);
    auto *item = image.isNull()
        ? new QGraphicsPixmapItem(QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation, nullptr, this).pixmap(128))
        : new QGraphicsPixmapItem(QPixmap::fromImage(image));
    m_iconScene->addItem(item);
    m_ui->graphicsView->setScene(m_iconScene);
    auto setQtVersion = [this] { m_ui->qtVersionLabel->setText(tr("Using <a href=\"qtversion\">Qt %1</a>").arg(QString::fromUtf8(qVersion()))); };
    setQtVersion();
    connect(this, &AboutDialog::retranslationRequired, this, std::move(setQtVersion));
    connect(m_ui->qtVersionLabel, &QLabel::linkActivated, this, &AboutDialog::linkActivated);
    centerWidget(this, parentWidget());
}

/*!
 * \brief Constructs an about dialog with the specified information.
 */
AboutDialog::AboutDialog(QWidget *parent, const QString &applicationName, const QString &creator, const QString &version, const QString &website,
    const QString &description, const QImage &image)
    : AboutDialog(parent, applicationName, creator, version, {}, website, description, image)
{
}

/*!
 * \brief Constructs an about dialog with the specified \a parent, \a
 * description and \a image.
 */
AboutDialog::AboutDialog(QWidget *parent, const QString &website, const QString &description, const QImage &image)
    : AboutDialog(parent, QString(), QString(), QString(), website, description, image)
{
}

/*!
 * \brief Destroys the about dialog.
 */
AboutDialog::~AboutDialog()
{
}

bool AboutDialog::event(QEvent *event)
{
    const auto res = QDialog::event(event);
    switch (event->type()) {
    case QEvent::PaletteChange:
        setStyleSheet(dialogStyleForPalette(palette()));
        break;
    case QEvent::LanguageChange:
        setWindowTitle(QCoreApplication::translate("QtUtilities::AboutDialog", "About", nullptr));
        emit retranslationRequired();
        break;
    default:;
    }
    return res;
}

void AboutDialog::linkActivated(const QString &link)
{
    if (link == QLatin1String("qtversion")) {
        QMessageBox::aboutQt(nullptr);
    }
}

} // namespace QtUtilities
