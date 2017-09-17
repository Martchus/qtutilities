#ifndef DIALOGS_OPTIONSCATEGORY_H
#define DIALOGS_OPTIONSCATEGORY_H

#include "../global.h"

#include <QIcon>
#include <QList>
#include <QObject>

namespace Dialogs {

class OptionPage;

class QT_UTILITIES_EXPORT OptionCategory : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString displayName READ displayName WRITE setDisplayName NOTIFY displayNameChanged)
    Q_PROPERTY(QIcon icon READ icon WRITE setIcon NOTIFY iconChanged)
    Q_PROPERTY(QList<OptionPage *> pages READ pages WRITE assignPages NOTIFY pagesChanged)

public:
    explicit OptionCategory(QObject *parent = nullptr);
    ~OptionCategory();

    const QString &displayName() const;
    void setDisplayName(const QString &displayName);
    const QIcon &icon() const;
    void setIcon(const QIcon &icon);
    const QList<OptionPage *> pages() const;
    void assignPages(const QList<OptionPage *> pages);
    bool applyAllPages();
    void resetAllPages();
    bool matches(const QString &searchKeyWord) const;
    int currentIndex() const;
    void setCurrentIndex(int currentIndex);

Q_SIGNALS:
    void displayNameChanged();
    void iconChanged();
    void pagesChanged();

private:
    QString m_displayName;
    QIcon m_icon;
    QList<OptionPage *> m_pages;
    int m_currentIndex;
};

/*!
 * \brief Returns the display name of the category.
 */
inline const QString &OptionCategory::displayName() const
{
    return m_displayName;
}

/*!
 * \brief Sets the display name of the category.
 */
inline void OptionCategory::setDisplayName(const QString &displayName)
{
    m_displayName = displayName;
    emit displayNameChanged();
}

/*!
 * \brief Returns the icon of the category.
 */
inline const QIcon &OptionCategory::icon() const
{
    return m_icon;
}

/*!
 * \brief Sets the icon of the category.
 */
inline void OptionCategory::setIcon(const QIcon &icon)
{
    m_icon = icon;
    emit iconChanged();
}

/*!
 * \brief Returns the assigned pages.
 */
inline const QList<OptionPage *> OptionCategory::pages() const
{
    return m_pages;
}

/*!
 * \brief Returns the index of the currently shown page.
 * \remarks The returned index might be invalid/out of range.
 * \sa setCurrentIndex()
 */
inline int OptionCategory::currentIndex() const
{
    return m_currentIndex;
}

/*!
 * \brief Sets the current index.
 * \sa currentIndex()
 */
inline void OptionCategory::setCurrentIndex(int currentIndex)
{
    m_currentIndex = currentIndex;
}
} // namespace Dialogs

#endif // DIALOGS_OPTIONSCATEGORY_H
