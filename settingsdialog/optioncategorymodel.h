#ifndef DIALOGS_OPTIONCATEGORYMODEL_H
#define DIALOGS_OPTIONCATEGORYMODEL_H

#include "../global.h"

#include <QAbstractListModel>
#include <QList>

namespace QtUtilities {

class OptionPage;
class OptionCategory;

class QT_UTILITIES_EXPORT OptionCategoryModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QList<OptionCategory *> categories READ categories WRITE setCategories)
public:
    explicit OptionCategoryModel(QObject *parent = nullptr);
    explicit OptionCategoryModel(const QList<OptionCategory *> &categories, QObject *parent = nullptr);
    ~OptionCategoryModel() override;

    const QList<OptionCategory *> &categories() const;
    OptionCategory *category(const QModelIndex &index) const;
    OptionCategory *category(int row) const;
    void setCategories(const QList<OptionCategory *> &categories);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private Q_SLOTS:
    void categoryChangedName();
    void categoryChangedIcon();

private:
    QList<OptionCategory *> m_categories;
};

/*!
 * \brief Returns the categories.
 * \sa OptionCategoryModel::category()
 * \sa OptionCategoryModel::setCategories()
 */
inline const QList<OptionCategory *> &OptionCategoryModel::categories() const
{
    return m_categories;
}

/*!
 * \brief Returns the category for the specified model \a index.
 * \sa OptionCategoryModel::categories()
 * \sa OptionCategoryModel::setCategories()
 */
inline OptionCategory *OptionCategoryModel::category(const QModelIndex &index) const
{
    return (index.isValid()) ? category(index.row()) : nullptr;
}

/*!
 * \brief Returns the category for the specified \a row.
 * \sa OptionCategoryModel::categories()
 * \sa OptionCategoryModel::setCategories()
 */
inline OptionCategory *OptionCategoryModel::category(int row) const
{
    return row < m_categories.size() ? m_categories.at(row) : nullptr;
}
} // namespace QtUtilities

#endif // DIALOGS_OPTIONCATEGORYMODEL_H
