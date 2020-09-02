#ifndef DIALOGS_OPTIONSCATEGORY_H
#define DIALOGS_OPTIONSCATEGORY_H

#include "../global.h"

#include <QIcon>
#include <QList>
#include <QObject>

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
Q_MOC_INCLUDE("settingsdialog/optionpage.h")
#endif

namespace QtUtilities {

class OptionPage;

class QT_UTILITIES_EXPORT OptionCategory : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString displayName READ displayName WRITE setDisplayName NOTIFY displayNameChanged)
    Q_PROPERTY(QIcon icon READ icon WRITE setIcon NOTIFY iconChanged)
    Q_PROPERTY(QList<OptionPage *> pages READ pages WRITE assignPages NOTIFY pagesChanged)

public:
    explicit OptionCategory(QObject *parent = nullptr);
    ~OptionCategory() override;

    const QString &displayName() const;
    void setDisplayName(const QString &displayName);
    const QIcon &icon() const;
    void setIcon(const QIcon &icon);
    const QList<OptionPage *> &pages() const;
    void assignPages(const QList<OptionPage *> &pages);
    bool applyAllPages();
    void resetAllPages();
    bool matches(const QString &searchKeyWord) const;
    int currentIndex() const;
    void setCurrentIndex(int currentIndex);

Q_SIGNALS:
    void displayNameChanged(const QString &displayName);
    void iconChanged(const QIcon &icon);
    void pagesChanged(const QList<OptionPage *> &pages);

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
    emit displayNameChanged(m_displayName = displayName);
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
    emit iconChanged(m_icon = icon);
}

/*!
 * \brief Returns the assigned pages.
 */
inline const QList<OptionPage *> &OptionCategory::pages() const
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
} // namespace QtUtilities

#endif // DIALOGS_OPTIONSCATEGORY_H
