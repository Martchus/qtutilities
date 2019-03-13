#ifndef WIDGETS_PALETTEEDITOR_H
#define WIDGETS_PALETTEEDITOR_H

#include "../global.h"

#include <QDialog>
#include <QItemDelegate>

#include <memory>

QT_FORWARD_DECLARE_CLASS(QListView)
QT_FORWARD_DECLARE_CLASS(QLabel)

namespace Widgets {
class ColorButton;
}

namespace Dialogs {

namespace Ui {
class PaletteEditor;
}

/*!
 * \brief The PaletteEditor class provides a dialog to customize a QPalette.
 *
 * This is taken from
 * qttools/src/designer/src/components/propertyeditor/paletteeditor.cpp.
 * In contrast to the original version this version doesn't provide a preview.
 */
class QT_UTILITIES_EXPORT PaletteEditor : public QDialog {
    Q_OBJECT
public:
    PaletteEditor(QWidget *parent);
    ~PaletteEditor() override;

    static QPalette getPalette(QWidget *parent, const QPalette &init = QPalette(), const QPalette &parentPal = QPalette(), int *result = nullptr);

    QPalette palette() const;
    void setPalette(const QPalette &palette);
    void setPalette(const QPalette &palette, const QPalette &parentPalette);

private Q_SLOTS:
    void handleBuildButtonColorChanged(const QColor &);
    void handleActiveRadioClicked();
    void handleInactiveRadioClicked();
    void handleDisabledRadioClicked();
    void handleComputeRadioClicked();
    void handleDetailsRadioClicked();

    void paletteChanged(const QPalette &palette);

private:
    void buildPalette();

    void updatePreviewPalette();
    void updateStyledButton();

    QPalette::ColorGroup currentColorGroup() const
    {
        return m_currentColorGroup;
    }

    std::unique_ptr<Ui::PaletteEditor> m_ui;
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
class QT_UTILITIES_EXPORT PaletteModel : public QAbstractTableModel {
    Q_OBJECT
    Q_PROPERTY(QPalette::ColorRole colorRole READ colorRole)
public:
    explicit PaletteModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QPalette getPalette() const;
    void setPalette(const QPalette &palette, const QPalette &parentPalette);

    QPalette::ColorRole colorRole() const
    {
        return QPalette::NoRole;
    }
    void setCompute(bool on)
    {
        m_compute = on;
    }

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
class QT_UTILITIES_EXPORT BrushEditor : public QWidget {
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
class QT_UTILITIES_EXPORT RoleEditor : public QWidget {
    Q_OBJECT
public:
    explicit RoleEditor(QWidget *parent = nullptr);

    void setLabel(const QString &label);
    void setEdited(bool on);
    bool edited() const;

Q_SIGNALS:
    void changed(QWidget *widget);

private Q_SLOTS:
    void emitResetProperty();

private:
    QLabel *m_label;
    bool m_edited;
};

/*!
 * \brief The ColorDelegate class is used by PaletteEditor.
 */
class QT_UTILITIES_EXPORT ColorDelegate : public QItemDelegate {
    Q_OBJECT

public:
    explicit ColorDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setEditorData(QWidget *ed, const QModelIndex &index) const override;
    void setModelData(QWidget *ed, QAbstractItemModel *model, const QModelIndex &index) const override;

    void updateEditorGeometry(QWidget *ed, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void paint(QPainter *painter, const QStyleOptionViewItem &opt, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &opt, const QModelIndex &index) const override;
};
} // namespace Dialogs

#endif // WIDGETS_PALETTEEDITOR_H
