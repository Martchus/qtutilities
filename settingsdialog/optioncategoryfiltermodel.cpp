#include "./optioncategoryfiltermodel.h"
#include "./optioncategory.h"
#include "./optioncategorymodel.h"

namespace Dialogs {

/*!
 * \class Dialogs::OptionCategoryFilterModel
 * \brief The OptionCategoryFilterModel class is used by SettingsDialog to
 * filter option categories.
 */

/*!
 * \brief Constructs an option category filter model.
 */
OptionCategoryFilterModel::OptionCategoryFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

bool OptionCategoryFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent))
        return true;
    if (OptionCategoryModel *model = qobject_cast<OptionCategoryModel *>(sourceModel())) {
        if (OptionCategory *category = model->category(sourceRow)) {
            return category->matches(filterRegExp().pattern());
        }
    }
    return false;
}
}
