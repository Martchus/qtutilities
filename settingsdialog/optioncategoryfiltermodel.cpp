#include "./optioncategoryfiltermodel.h"
#include "./optioncategory.h"
#include "./optioncategorymodel.h"

namespace QtUtilities {

/*!
 * \class OptionCategoryFilterModel
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
    if (auto *const model = qobject_cast<OptionCategoryModel *>(sourceModel())) {
        if (OptionCategory *category = model->category(sourceRow)) {
            return category->matches(
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
                filterRegularExpression().pattern()
#elif (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
                !filterRegularExpression().pattern().isEmpty() ? filterRegularExpression().pattern() : filterRegExp().pattern()
#else
                filterRegExp().pattern()
#endif
            );
        }
    }
    return false;
}
} // namespace QtUtilities
