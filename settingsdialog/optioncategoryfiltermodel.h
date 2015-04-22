#ifndef OPTIONCATEGORYFILTERMODEL_H
#define OPTIONCATEGORYFILTERMODEL_H

#include <QSortFilterProxyModel>

namespace Dialogs {

class OptionCategoryFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit OptionCategoryFilterModel(QObject *parent = nullptr);

protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &sourceParent) const;
};


}


#endif // OPTIONCATEGORYFILTERMODEL_H
