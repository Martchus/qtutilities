#ifndef PALETTEEDITOR_H
#define PALETTEEDITOR_H

#include "ui_paletteeditor.h"

#include <c++utilities/application/global.h>

#include <QItemDelegate>

QT_FORWARD_DECLARE_CLASS(QListView)
QT_FORWARD_DECLARE_CLASS(QLabel)

namespace Widgets {
class ColorButton;
}

namespace Dialogs {

/*!
 * \brief The PaletteEditor class provides a dialog to customize a QPalette.
 *
 * This is taken from qttools/src/designer/src/components/propertyeditor/paletteeditor.cpp.
 * In contrast to the original version this version doesn't provide a preview.
 */
class LIB_EXPORT PaletteEditor : public QDialog
{
    Q_OBJECT
public:
    PaletteEditor(QWidget *parent);
    ~PaletteEditor();

    static QPalette getPalette(QWidget *parent, const QPalette &init = QPalette(),
                const QPalette &parentPal = QPalette(), int *result = nullptr);

    QPalette palette() const;
    void setPalette(const QPalette &palette);
    void setPalette(const QPalette &palette, const QPalette &parentPalette);

private Q_SLOTS:
    void on_buildButton_colorChanged(const QColor &);
    void on_activeRadio_clicked();
    void on_inactiveRadio_clicked();
    void on_disabledRadio_clicked();
    void on_computeRadio_clicked();
    void on_detailsRadio_clicked();

    void paletteChanged(const QPalette &palette);

private:
    void buildPalette();

    void updatePreviewPalette();
    void updateStyledButton();

    QPalette::ColorGroup currentColorGroup() const
    {
        return m_currentColorGroup;
    }

    Ui::PaletteEditor m_ui;
    QPalette m_editPalette;
    QPalette m_parentPalette;
    QPalette::ColorGroup m_currentColorGroup;
    class PaletteModel *m_paletteModel;
    bool m_modelUpdated;
    bool m_paletteUpdated;
    bool m_compute;
};

/*!
 * \brief The PaletteModel class is used by PaletteEditor.
 */
class LIB_EXPORT PaletteModel : public QAbstractTableModel
{
    Q_OBJECT
    Q_PROPERTY(QPalette::ColorRole colorRole READ colorRole)
public:
    explicit PaletteModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                int role = Qt::DisplayRole) const;

    QPalette getPalette() const;
    void setPalette(const QPalette &palette, const QPalette &parentPalette);

    QPalette::ColorRole colorRole() const { return QPalette::NoRole; }
    void setCompute(bool on) { m_compute = on; }

Q_SIGNALS:
    void paletteChanged(const QPalette &palette);

private:

    QPalette::ColorGroup columnToGroup(int index) const;
    int groupToColumn(QPalette::ColorGroup group) const;

    QPalette m_palette;
    QPalette m_parentPalette;
    QMap<QPalette::ColorRole, QString> m_roleNames;
    bool m_compute;
};

/*!
 * \brief The BrushEditor class is used by PaletteEditor.
 */
class LIB_EXPORT BrushEditor : public QWidget
{
    Q_OBJECT

public:
    explicit BrushEditor(QWidget *parent = nullptr);

    void setBrush(const QBrush &brush);
    QBrush brush() const;
    bool changed() const;

Q_SIGNALS:
    void changed(QWidget *widget);

private Q_SLOTS:
    void brushChanged();

private:
    Widgets::ColorButton *m_button;
    bool m_changed;
};

/*!
 * \brief The RoleEditor class is used by PaletteEditor.
 */
class LIB_EXPORT RoleEditor : public QWidget
{
    Q_OBJECT
public:
    explicit RoleEditor(QWidget *parent = nullptr);

    void setLabel(const QString &label);
    void setEdited(bool on);
    bool edited() const;

signals:
    void changed(QWidget *widget);

private slots:
    void emitResetProperty();

private:
    QLabel *m_label;
    bool m_edited;
};

/*!
 * \brief The ColorDelegate class is used by PaletteEditor.
 */
class LIB_EXPORT ColorDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    explicit ColorDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                const QModelIndex &index) const;

    void setEditorData(QWidget *ed, const QModelIndex &index) const;
    void setModelData(QWidget *ed, QAbstractItemModel *model,
                const QModelIndex &index) const;

    void updateEditorGeometry(QWidget *ed,
                const QStyleOptionViewItem &option, const QModelIndex &index) const;

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &opt,
                       const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &opt, const QModelIndex &index) const Q_DECL_OVERRIDE;
};

}

#endif // PALETTEEDITOR_H
