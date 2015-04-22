#ifndef OPTIONCATEGORYMODEL_H
#define OPTIONCATEGORYMODEL_H

#include <c++utilities/application/global.h>

#include <QList>
#include <QAbstractListModel>

namespace Dialogs {

class OptionPage;
class OptionCategory;

class LIB_EXPORT OptionCategoryModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit OptionCategoryModel(QObject *parent = nullptr);
    explicit OptionCategoryModel(const QList<OptionCategory *> &categories, QObject *parent = nullptr);
    virtual ~OptionCategoryModel();
    
    const QList<OptionCategory *> &categories() const;
    OptionCategory *category(const QModelIndex &index) const;
    OptionCategory *category(int row) const;
    void setCategories(const QList<OptionCategory *> categories);
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

private slots:
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
    return (index.isValid())
            ? category(index.row())
            : nullptr;
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

}

#endif // OPTIONCATEGORYMODEL_H
