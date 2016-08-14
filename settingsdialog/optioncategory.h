#ifndef DIALOGS_OPTIONSCATEGORY_H
#define DIALOGS_OPTIONSCATEGORY_H

#include <c++utilities/application/global.h>

#include <QObject>
#include <QIcon>
#include <QList>

namespace Dialogs {

class OptionPage;

class LIB_EXPORT OptionCategory : public QObject
{
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
    
Q_SIGNALS:
    void displayNameChanged();
    void iconChanged();
    void pagesChanged();

private:
    QString m_displayName;
    QIcon m_icon;
    QList<OptionPage *> m_pages;
    
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

}

#endif // DIALOGS_OPTIONSCATEGORY_H
