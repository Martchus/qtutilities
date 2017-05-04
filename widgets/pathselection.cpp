#include "./pathselection.h"
#include "./clearlineedit.h"

#include "../misc/desktoputils.h"

#include <c++utilities/io/path.h>

#include <QCompleter>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QMenu>
#include <QPushButton>
#include <QStringBuilder>
#ifndef QT_NO_CONTEXTMENU
#include <QContextMenuEvent>
#endif

#include <memory>

using namespace std;

namespace Widgets {

/*!
 * \class Widgets::PathSelection
 * \brief A QLineEdit with a QPushButton next to it which allows to select
 * file/directory via QFileDialog.
 */

QCompleter *PathSelection::m_completer = nullptr;

/*!
 * \brief Constructs a path selection widget.
 */
PathSelection::PathSelection(QWidget *parent)
    : QWidget(parent)
    , m_lineEdit(new ClearLineEdit(this))
    , m_button(new QPushButton(this))
    , m_customMode(QFileDialog::Directory)
    , m_customDialog(nullptr)
{
    if (!m_completer) {
        auto *fileSystemModel = new QFileSystemModel(m_completer);
        fileSystemModel->setRootPath(QString());
        m_completer = new QCompleter;
        m_completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
        m_completer->setModel(fileSystemModel);
    }

    m_lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_lineEdit->installEventFilter(this);
    m_lineEdit->setCompleter(m_completer);
    m_button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_button->setText(tr("Select ..."));

    auto *layout = new QHBoxLayout(this);
    layout->setSpacing(3);
    layout->setMargin(0);
    layout->addWidget(m_lineEdit);
    layout->addWidget(m_button);
    setLayout(layout);

    connect(m_button, &QPushButton::clicked, this, &PathSelection::showFileDialog);
}

bool PathSelection::eventFilter(QObject *obj, QEvent *event)
{
#ifndef QT_NO_CONTEXTMENU
    if (obj == m_lineEdit) {
        switch (event->type()) {
        case QEvent::ContextMenu: {
            unique_ptr<QMenu> menu(m_lineEdit->createStandardContextMenu());
            menu->addSeparator();
            connect(menu->addAction(QIcon::fromTheme(QStringLiteral("document-open")), tr("Select ...")), &QAction::triggered, this,
                &PathSelection::showFileDialog);
            QFileInfo fileInfo(m_lineEdit->text());
            if (fileInfo.exists()) {
                if (fileInfo.isFile()) {
                    connect(menu->addAction(QIcon::fromTheme(QStringLiteral("system-run")), tr("Open")), &QAction::triggered,
                        bind(&DesktopUtils::openLocalFileOrDir, m_lineEdit->text()));
                } else if (fileInfo.isDir()) {
                    connect(menu->addAction(QIcon::fromTheme(QStringLiteral("system-file-manager")), tr("Explore")), &QAction::triggered,
                        bind(&DesktopUtils::openLocalFileOrDir, m_lineEdit->text()));
                }
            }
            menu->exec(static_cast<QContextMenuEvent *>(event)->globalPos());
        }
            return true;
        default:;
        }
    }
#endif
    return QWidget::eventFilter(obj, event);
}

void PathSelection::showFileDialog()
{
    QString directory;
    QFileInfo fileInfo(m_lineEdit->text());
    if (fileInfo.exists()) {
        if (fileInfo.isFile()) {
            directory = fileInfo.absoluteDir().absolutePath();
        } else {
            directory = fileInfo.absolutePath();
        }
    }
    if (m_customDialog) {
        m_customDialog->setDirectory(directory);
        if (m_customDialog->exec() == QFileDialog::Accepted) {
            m_lineEdit->selectAll();
            m_lineEdit->insert(m_customDialog->selectedFiles().join(SEARCH_PATH_SEP_CHAR));
        }
    } else {
        QFileDialog dialog(this);
        dialog.setDirectory(directory);
        dialog.setFileMode(m_customMode);
        if (window()) {
            dialog.setWindowTitle(tr("Select path") % QStringLiteral(" - ") % window()->windowTitle());
        } else {
            dialog.setWindowTitle(tr("Select path"));
        }
        if (dialog.exec() == QFileDialog::Accepted) {
            m_lineEdit->selectAll();
            m_lineEdit->insert(dialog.selectedFiles().join(SEARCH_PATH_SEP_CHAR));
        }
    }
}
}
