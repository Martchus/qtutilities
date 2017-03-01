#include "recentmenumanager.h"

#include <QStringList>
#include <QCoreApplication>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QPushButton>
#include <QFile>

namespace MiscUtils {

/*!
 * \class RecentMenuManager
 * \brief The RecentMenuManager class manages the entries for a "recently opened files" menu.
 */

/*!
 * \brief Constructs a new recent menu manager.
 * \param menu Specifies the QMenu instance to operate with.
 * \param parent Specifies the parent QObject; might be nullptr.
 * \remarks
 *  - Menu title and icon are set within the constructor.
 *  - The current menu entries are cleared.
 *  - The menu entries shouldn't be manipulated manually by the caller till the manager is destructed.
 *  - The manager does not take ownership over \a menu.
 */
RecentMenuManager::RecentMenuManager(QMenu *menu, QObject *parent) :
    QObject(parent),
    m_menu(menu)
{
    m_menu->clear();
    m_menu->setTitle(tr("&Recent"));
    m_menu->setIcon(QIcon::fromTheme(QStringLiteral("document-open-recent")));
    m_sep = m_menu->addSeparator();
    m_clearAction = m_menu->addAction(QIcon::fromTheme(QStringLiteral("edit-clear")), tr("&Clear list"), this, &RecentMenuManager::clearEntries);
}

/*!
 * \brief Restores the specified entries.
 */
void RecentMenuManager::restore(const QStringList &savedEntries)
{
    QAction *action = nullptr;
    for(const QString &path : savedEntries) {
        if(!path.isEmpty()) {
            action = new QAction(path, m_menu);
            action->setProperty("file_path", path);
            m_menu->insertAction(m_sep, action);
            connect(action, &QAction::triggered, this, &RecentMenuManager::handleActionTriggered);
        }
    }
    if(action) {
        m_menu->actions().front()->setShortcut(QKeySequence(Qt::Key_F6));
        m_menu->setEnabled(true);
    }
}

/*!
 * \brief Saves the current entries.
 */
QStringList RecentMenuManager::save()
{
    QStringList existingEntires;
    QList<QAction *> entryActions = m_menu->actions();
    existingEntires.reserve(entryActions.size());
    for(const QAction *action : entryActions) {
        QVariant path = action->property("file_path");
        if(!path.isNull()) {
            existingEntires << path.toString();
        }
    }
    return existingEntires;
}

/*!
 * \brief Ensures an entry for the specified \a path is present and the first entry in the list.
 */
void RecentMenuManager::addEntry(const QString &path)
{
    QList<QAction *> existingEntries = m_menu->actions();
    QAction *entry = nullptr;
    // remove shortcut from existing entries
    for(QAction *existingEntry : existingEntries) {
        existingEntry->setShortcut(QKeySequence());
        // check whether existing entry matches entry to add
        if(existingEntry->property("file_path").toString() == path) {
            entry = existingEntry;
            break;
        }
    }
    if(!entry) {
        // remove old entries to have never more than 10 entries
        for(int i = existingEntries.size() - 1; i > 8; --i) {
            delete existingEntries[i];
        }
        existingEntries = m_menu->actions();
        // create new action
        entry = new QAction(path, this);
        entry->setProperty("file_path", path);
        connect(entry, &QAction::triggered, this, &RecentMenuManager::handleActionTriggered);
    } else {
        // remove existing action (will be inserted again as first action)
        m_menu->removeAction(entry);
    }
    // add shortcut for new entry
    entry->setShortcut(QKeySequence(Qt::Key_F6));
    // ensure menu is enabled
    m_menu->setEnabled(true);
    // add action as first action in the recent menu
    m_menu->insertAction(m_menu->isEmpty() ? nullptr : m_menu->actions().front(), entry);
}

/*!
 * \brief Clears all entries.
 */
void RecentMenuManager::clearEntries()
{
    QList<QAction *> entries = m_menu->actions();
    for(auto i = entries.begin(), end = entries.end() - 2; i != end; ++i) {
        if(*i != m_clearAction) {
            delete *i;
        }
    }
    m_menu->setEnabled(false);
}

/*!
 * \brief Internally called to emit fileSelected() after an action has been triggered.
 */
void RecentMenuManager::handleActionTriggered()
{
    if(QAction *action = qobject_cast<QAction *>(sender())) {
        const QString path = action->property("file_path").toString();
        if(!path.isEmpty()) {
            if(QFile::exists(path)) {
                emit fileSelected(path);
            } else {
                QMessageBox msg;
                msg.setWindowTitle(tr("Recently opened files - ") + QCoreApplication::applicationName());
                msg.setText(tr("The selected file can't be found anymore. Do you want to delete the obsolete entry from the list?"));
                msg.setIcon(QMessageBox::Warning);
                QPushButton *keepEntryButton = msg.addButton(tr("keep entry"), QMessageBox::NoRole);
                QPushButton *deleteEntryButton = msg.addButton(tr("delete entry"), QMessageBox::YesRole);
                msg.setEscapeButton(keepEntryButton);
                msg.exec();
                if(msg.clickedButton() == deleteEntryButton) {
                    delete action;
                    QList<QAction *> remainingActions = m_menu->actions();
                    if(!remainingActions.isEmpty() && remainingActions.front() != m_sep && remainingActions.front() != m_clearAction) {
                        remainingActions.front()->setShortcut(QKeySequence(Qt::Key_F6));
                        m_menu->setEnabled(true);
                    } else {
                        m_menu->setEnabled(false);
                    }
                }
            }
        }
    }
}

/*!
 * \fn RecentMenuManager::fileSelected()
 * \brief Emitted after the user selected a file.
 * \remarks Only emitted when the selected file still existed; otherwise the user is ask whether to keep or delete the entry.
 */

}
