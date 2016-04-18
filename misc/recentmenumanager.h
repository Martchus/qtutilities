#ifndef RECENTMENUMANAGER_H
#define RECENTMENUMANAGER_H

#include <c++utilities/application/global.h>

#include <QObject>

QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QAction)

namespace MiscUtils {

class LIB_EXPORT RecentMenuManager : public QObject
{
    Q_OBJECT

public:
    RecentMenuManager(QMenu *menu, QObject *parent = nullptr);

    void restore(const QStringList &savedEntries);
    QStringList save();

public Q_SLOTS:
    void addEntry(const QString &path);
    void clearEntries();

Q_SIGNALS:
    /*!
     * \brief Emitted after the user selected a file.
     * \remarks Only emitted when the selected file still existed; otherwise the user is ask whether to keep or delete the entry.
     */
    void fileSelected(const QString &path);

private Q_SLOTS:
    void handleActionTriggered();

private:
    QMenu *m_menu;
    QAction *m_sep;
    QAction *m_clearAction;
};

}

#endif // RECENTMENUMANAGER_H
