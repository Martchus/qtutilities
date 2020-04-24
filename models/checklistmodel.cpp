#include "./checklistmodel.h"

#include <QSettings>

/*!
    \namespace Models
    \brief Provides common models.
*/

namespace QtUtilities {

/*!
 * \class Models::ChecklistItem
 * \brief The ChecklistItem class provides an item for use with the
 * ChecklistModel class.
 */

/*!
 * \class Models::ChecklistModel
 * \brief The ChecklistModel class provides a generic model for storing
 * checkable items.
 */

/*!
 * \brief Constructs a new checklist model.
 */
ChecklistModel::ChecklistModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ChecklistModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return m_items.size();
    }
    return 0;
}

Qt::ItemFlags ChecklistModel::flags(const QModelIndex &index) const
{
    if (!index.isValid() || index.row() >= m_items.count() || index.model() != this) {
        return Qt::ItemIsDropEnabled; // allows drops outside the items
    }
    return QAbstractListModel::flags(index) | Qt::ItemIsUserCheckable | Qt::ItemIsDragEnabled;
}

QVariant ChecklistModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && index.row() < m_items.size()) {
        switch (role) {
        case Qt::DisplayRole:
            return m_items.at(index.row()).label();
        case Qt::CheckStateRole:
            return m_items.at(index.row()).checkState();
        case idRole():
            return m_items.at(index.row()).id();
        default:;
        }
    }
    return QVariant();
}

QMap<int, QVariant> ChecklistModel::itemData(const QModelIndex &index) const
{
    QMap<int, QVariant> roles;
    roles.insert(Qt::DisplayRole, data(index, Qt::DisplayRole));
    roles.insert(Qt::CheckStateRole, data(index, Qt::CheckStateRole));
    roles.insert(idRole(), data(index, idRole()));
    return roles;
}

bool ChecklistModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    bool success = false;
    QVector<int> roles{ role };
    if (index.isValid() && index.row() < m_items.size()) {
        switch (role) {
        case Qt::DisplayRole:
            m_items[index.row()].m_label = value.toString();
            success = true;
            break;
        case Qt::CheckStateRole:
            if (value.canConvert(QMetaType::Int)) {
                m_items[index.row()].m_checkState = static_cast<Qt::CheckState>(value.toInt());
                success = true;
            }
            break;
        case idRole(): {
            m_items[index.row()].m_id = value;
            success = true;
            auto label = labelForId(value);
            if (!label.isEmpty()) {
                m_items[index.row()].m_label = std::move(label);
                roles << Qt::DisplayRole;
            }
            break;
        }
        default:;
        }
    }
    if (success) {
        dataChanged(index, index, roles);
    }
    return success;
}

bool ChecklistModel::setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles)
{
    for (QMap<int, QVariant>::ConstIterator it = roles.constBegin(); it != roles.constEnd(); ++it) {
        setData(index, it.value(), it.key());
    }
    return true;
}

/*!
 * \brief Sets the checked state of the specified item.
 */
bool ChecklistModel::setChecked(int row, Qt::CheckState checked)
{
    if (row < 0 || row >= m_items.size()) {
        return false;
    }
    m_items[row].m_checkState = checked ? Qt::Checked : Qt::Unchecked;
    const auto index(this->index(row));
    dataChanged(index, index, QVector<int>{ Qt::CheckStateRole });
    return true;
}

/*!
 * \brief Returns the label for the specified \a id.
 *
 * This method might be reimplemented when subclassing to provide labels
 * for the item IDs.
 *
 * If an item's ID is set (using setData() with idRole() or setItems()) this method
 * is called to update or initialize the item's label as well. If this method returns
 * an empty string (default behaviour) the item's label will not be updated.
 *
 * This is useful when items are moved by the view (eg. for Drag & Drop) and to
 * initialize the ChecklistItem labels more conveniently.
 */
QString ChecklistModel::labelForId(const QVariant &) const
{
    return QString();
}

Qt::DropActions ChecklistModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

bool ChecklistModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (count < 1 || row < 0 || row > rowCount() || parent.isValid()) {
        return false;
    }
    beginInsertRows(QModelIndex(), row, row + count - 1);
    for (int index = row, end = row + count; index < end; ++index) {
        m_items.insert(index, ChecklistItem());
    }
    endInsertRows();
    return true;
}

bool ChecklistModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (count < 1 || row < 0 || (row + count) > rowCount() || parent.isValid()) {
        return false;
    }
    beginRemoveRows(QModelIndex(), row, row + count - 1);
    for (int index = row, end = row + count; index < end; ++index) {
        m_items.removeAt(index);
    }
    endRemoveRows();
    return true;
}

/*!
 * \brief Sets the items. Resets the model.
 */
void ChecklistModel::setItems(const QList<ChecklistItem> &items)
{
    beginResetModel();
    m_items = items;
    for (auto &item : m_items) {
        if (item.m_label.isEmpty()) {
            item.m_label = labelForId(item.id());
        }
    }
    endResetModel();
}

/*!
 * \brief Restores the IDs and checkstates read from the specified \a settings
 * object.
 *
 * The items will be read from the array with the specified \a name.
 *
 * Resets the model (current items are cleared).
 *
 * Does not restore any labels. Labels are meant to be restored from the ID.
 */
void ChecklistModel::restore(QSettings &settings, const QString &name)
{
    beginResetModel();
    auto currentItems = m_items;
    QList<QVariant> restoredIds;
    m_items.clear();
    int rows = settings.beginReadArray(name);
    m_items.reserve(rows);
    for (int i = 0; i < rows; ++i) {
        settings.setArrayIndex(i);
        const auto id = settings.value(QStringLiteral("id"));
        const auto isIdValid = [&] {
            for (const auto &item : currentItems) {
                if (item.id() == id) {
                    return true;
                }
            }
            return false;
        }();
        if (!isIdValid) {
            continue;
        }
        const auto selected = settings.value(QStringLiteral("selected"));
        if (!id.isNull() && !selected.isNull() && selected.canConvert(QMetaType::Bool) && !restoredIds.contains(id)) {
            m_items << ChecklistItem(id, labelForId(id), selected.toBool() ? Qt::Checked : Qt::Unchecked);
            restoredIds << id;
        }
    }
    settings.endArray();
    for (const ChecklistItem &item : currentItems) {
        if (!restoredIds.contains(item.id())) {
            m_items << item;
        }
    }
    endResetModel();
}

/*!
 * \brief Saves the IDs and checkstates to the specified \a settings object.
 *
 * The items will be stored using an array with the specified \a name.
 *
 * Does not save any labels.
 */
void ChecklistModel::save(QSettings &settings, const QString &name) const
{
    settings.beginWriteArray(name, m_items.size());
    int index = 0;
    for (const ChecklistItem &item : m_items) {
        settings.setArrayIndex(index);
        settings.setValue(QStringLiteral("id"), item.id());
        settings.setValue(QStringLiteral("selected"), item.isChecked());
        ++index;
    }
    settings.endArray();
}

/*!
 * \brief Returns the checked IDs.
 */
QVariantList ChecklistModel::toVariantList() const
{
    QVariantList checkedIds;
    checkedIds.reserve(m_items.size());
    for (const auto &item : m_items) {
        if (item.isChecked()) {
            checkedIds << item.id();
        }
    }
    return checkedIds;
}

/*!
 * \brief Checks all items contained by \a checkedIds and unchecks other items.
 */
void ChecklistModel::applyVariantList(const QVariantList &checkedIds)
{
    for (auto &item : m_items) {
        item.m_checkState = checkedIds.contains(item.id()) ? Qt::Checked : Qt::Unchecked;
    }
    emit dataChanged(index(0), index(m_items.size()), { Qt::CheckStateRole });
}

} // namespace QtUtilities
