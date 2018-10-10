#ifndef MODELS_CHECKLISTMODEL_H
#define MODELS_CHECKLISTMODEL_H

#include "../global.h"

#include <QAbstractListModel>
#include <QList>

QT_FORWARD_DECLARE_CLASS(QSettings)

namespace Models {

class ChecklistModel;

class QT_UTILITIES_EXPORT ChecklistItem {
    friend class ChecklistModel;

public:
    ChecklistItem(const QVariant &id = QVariant(), const QString &label = QString(), Qt::CheckState checked = Qt::Unchecked);

    const QVariant &id() const;
    const QString &label() const;
    Qt::CheckState checkState() const;
    bool isChecked() const;

private:
    QVariant m_id;
    QString m_label;
    Qt::CheckState m_checkState;
};

inline ChecklistItem::ChecklistItem(const QVariant &id, const QString &label, Qt::CheckState checkState)
    : m_id(id)
    , m_label(label)
    , m_checkState(checkState)
{
}

/*!
 * \brief Returns the ID of the item.
 */
inline const QVariant &ChecklistItem::id() const
{
    return m_id;
}

/*!
 * \brief Returns the label.
 */
inline const QString &ChecklistItem::label() const
{
    return m_label;
}

/*!
 * \brief Returns the check state.
 */
inline Qt::CheckState ChecklistItem::checkState() const
{
    return m_checkState;
}

/*!
 * \brief Returns whether the item is checked.
 */

inline bool ChecklistItem::isChecked() const
{
    return m_checkState == Qt::Checked;
}

class QT_UTILITIES_EXPORT ChecklistModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit ChecklistModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QMap<int, QVariant> itemData(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::DisplayRole) override;
    bool setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles) override;
    bool setChecked(int row, bool checked);
    bool setChecked(int row, Qt::CheckState checked);
    virtual QString labelForId(const QVariant &id) const;
    Qt::DropActions supportedDropActions() const override;
    bool insertRows(int row, int count, const QModelIndex &parent) override;
    bool removeRows(int row, int count, const QModelIndex &parent) override;
    const QList<ChecklistItem> &items() const;
    void setItems(const QList<ChecklistItem> &items);
    void restore(QSettings &settings, const QString &name);
    void save(QSettings &settings, const QString &name) const;
    QVariantList toVariantList() const;
    void applyVariantList(const QVariantList &checkedIds);
    static constexpr int idRole();

private:
    QList<ChecklistItem> m_items;
};

/*!
 * \brief Returns the items.
 */
inline const QList<ChecklistItem> &ChecklistModel::items() const
{
    return m_items;
}

/*!
 * \brief Sets the checked state of the specified item.
 */
inline bool ChecklistModel::setChecked(int row, bool checked)
{
    return setChecked(row, checked ? Qt::Checked : Qt::Unchecked);
}

/*!
 * \brief Returns the role used to get or set the item ID.
 */
constexpr int ChecklistModel::idRole()
{
    return Qt::UserRole + 1;
}
} // namespace Models

#endif // MODELS_CHECKLISTMODEL_H
