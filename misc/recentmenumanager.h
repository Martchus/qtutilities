#ifndef MISC_UTILS_RECENTMENUMANAGER_H
#define MISC_UTILS_RECENTMENUMANAGER_H

#include "../global.h"

#include <QObject>

QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QAction)

namespace MiscUtils {

class QT_UTILITIES_EXPORT RecentMenuManager : public QObject
{
    Q_OBJECT

public:
    RecentMenuManager(QMenu *menu, QObject *parent = nullptr);

public Q_SLOTS:
    void restore(const QStringList &savedEntries);
    QStringList save();
    void addEntry(const QString &path);
    void clearEntries();

Q_SIGNALS:
    void fileSelected(const QString &path);

private Q_SLOTS:
    void handleActionTriggered();

private:
    QMenu *m_menu;
    QAction *m_sep;
    QAction *m_clearAction;
};

}

#endif // MISC_UTILS_RECENTMENUMANAGER_H
