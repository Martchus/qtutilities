#include "./optioncategorymodel.h"
#include "./optioncategory.h"

#ifdef QT_UTILITIES_GUI_QTWIDGETS
#include <QApplication>
#include <QStyle>
#endif

namespace Dialogs {

/*!
 * \class Dialogs::OptionCategoryModel
 * \brief The OptionCategoryModel class is used by SettingsDialog to store and
 * display option categories.
 */

/*!
 * \brief Constructs an option category model.
 */
OptionCategoryModel::OptionCategoryModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

/*!
 * \brief Constructs an option category model with the specified \a categories.
 * \remarks The model takes ownership over the given categories.
 */
OptionCategoryModel::OptionCategoryModel(const QList<OptionCategory *> &categories, QObject *parent)
    : QAbstractListModel(parent)
    , m_categories(categories)
{
    for (OptionCategory *category : m_categories) {
        category->setParent(this);
    }
}

/*!
 * \brief Destroys the option category model.
 */
OptionCategoryModel::~OptionCategoryModel()
{
}

/*!
 * \brief Sets the \a categories for the model.
 *
 * The model takes ownership over the given \a categories.
 */
void OptionCategoryModel::setCategories(const QList<OptionCategory *> categories)
{
    beginResetModel();
    qDeleteAll(m_categories);
    m_categories = categories;
    for (OptionCategory *category : m_categories) {
        category->setParent(this);
        connect(category, &OptionCategory::displayNameChanged, this, &OptionCategoryModel::categoryChangedName);
        connect(category, &OptionCategory::iconChanged, this, &OptionCategoryModel::categoryChangedIcon);
    }
    endResetModel();
}

int OptionCategoryModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_categories.size();
}

QVariant OptionCategoryModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && index.row() < m_categories.size()) {
        switch (role) {
        case Qt::DisplayRole:
            return m_categories.at(index.row())->displayName();
        case Qt::DecorationRole: {
            const QIcon &icon = m_categories.at(index.row())->icon();
            if (!icon.isNull()) {
                return icon.pixmap(
#ifdef QT_UTILITIES_GUI_QTWIDGETS
                    QApplication::style()->pixelMetric(QStyle::PM_LargeIconSize)
#else
                    QSize(32, 32)
#endif
                        );
            }
        }
        }
    }
    return QVariant();
}

/*!
 * \brief Handles the change of name of a category.
 */
void OptionCategoryModel::categoryChangedName()
{
    if (OptionCategory *senderCategory = qobject_cast<OptionCategory *>(QObject::sender())) {
        for (int i = 0, end = m_categories.size(); i < end; ++i) {
            if (senderCategory == m_categories.at(i)) {
                QModelIndex index = this->index(i);
                emit dataChanged(index, index, QVector<int>() << Qt::DisplayRole);
            }
        }
    }
}

/*!
 * \brief Handles the a changed icon of a category.
 */
void OptionCategoryModel::categoryChangedIcon()
{
    if (OptionCategory *senderCategory = qobject_cast<OptionCategory *>(QObject::sender())) {
        for (int i = 0, end = m_categories.size(); i < end; ++i) {
            if (senderCategory == m_categories.at(i)) {
                QModelIndex index = this->index(i);
                emit dataChanged(index, index, QVector<int>() << Qt::DecorationRole);
            }
        }
    }
}
}
