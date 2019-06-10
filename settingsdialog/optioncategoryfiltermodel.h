#ifndef DIALOGS_OPTIONCATEGORYFILTERMODEL_H
#define DIALOGS_OPTIONCATEGORYFILTERMODEL_H

#include <QSortFilterProxyModel>

namespace QtUtilities {

class OptionCategoryFilterModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit OptionCategoryFilterModel(QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &sourceParent) const override;
};
} // namespace QtUtilities

#endif // DIALOGS_OPTIONCATEGORYFILTERMODEL_H
