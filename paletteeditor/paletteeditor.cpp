#include "./paletteeditor.h"
#include "./colorbutton.h"

#include "ui_paletteeditor.h"

#include <QHeaderView>
#include <QLabel>
#include <QMetaProperty>
#include <QPainter>
#include <QStyle>
#include <QToolButton>

namespace QtUtilities {

enum { BrushRole = 33 };

PaletteEditor::PaletteEditor(QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::PaletteEditor)
    , m_currentColorGroup(QPalette::Active)
    , m_paletteModel(new PaletteModel(this))
    , m_modelUpdated(false)
    , m_paletteUpdated(false)
    , m_compute(true)
{
    m_ui->setupUi(this);
    m_ui->paletteView->setModel(m_paletteModel);
    updatePreviewPalette();
    updateStyledButton();
    m_ui->paletteView->setModel(m_paletteModel);
    auto *const delegate = new ColorDelegate(this);
    m_ui->paletteView->setItemDelegate(delegate);
    m_ui->paletteView->setEditTriggers(QAbstractItemView::AllEditTriggers);
    connect(m_paletteModel, &PaletteModel::paletteChanged, this, &PaletteEditor::paletteChanged);
    m_ui->paletteView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ui->paletteView->setDragEnabled(true);
    m_ui->paletteView->setDropIndicatorShown(true);
    m_ui->paletteView->setRootIsDecorated(false);
    m_ui->paletteView->setColumnHidden(2, true);
    m_ui->paletteView->setColumnHidden(3, true);
}

PaletteEditor::~PaletteEditor()
{
}

QPalette PaletteEditor::palette() const
{
    return m_editPalette;
}

void PaletteEditor::setPalette(const QPalette &palette)
{
    m_editPalette = palette;
    const auto mask = palette.
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
                      resolveMask()
#else
                      resolve()
#endif
        ;
    for (int i = 0; i < static_cast<int>(QPalette::NColorRoles); ++i) {
        if (mask & (1 << i)) {
            continue;
        }
        m_editPalette.setBrush(
            QPalette::Active, static_cast<QPalette::ColorRole>(i), m_parentPalette.brush(QPalette::Active, static_cast<QPalette::ColorRole>(i)));
        m_editPalette.setBrush(
            QPalette::Inactive, static_cast<QPalette::ColorRole>(i), m_parentPalette.brush(QPalette::Inactive, static_cast<QPalette::ColorRole>(i)));
        m_editPalette.setBrush(
            QPalette::Disabled, static_cast<QPalette::ColorRole>(i), m_parentPalette.brush(QPalette::Disabled, static_cast<QPalette::ColorRole>(i)));
    }
    m_editPalette.
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        setResolveMask(mask);
    m_editPalette = m_editPalette.resolve(m_editPalette)
#else
        resolve(mask)
#endif
        ;
    updatePreviewPalette();
    updateStyledButton();
    m_paletteUpdated = true;
    if (!m_modelUpdated) {
        m_paletteModel->setPalette(m_editPalette, m_parentPalette);
    }
    m_paletteUpdated = false;
}

void PaletteEditor::setPalette(const QPalette &palette, const QPalette &parentPalette)
{
    m_parentPalette = parentPalette;
    setPalette(palette);
}

void PaletteEditor::handleBuildButtonColorChanged(const QColor &)
{
    buildPalette();
}

void PaletteEditor::handleActiveRadioClicked()
{
    m_currentColorGroup = QPalette::Active;
    updatePreviewPalette();
}

void PaletteEditor::handleInactiveRadioClicked()
{
    m_currentColorGroup = QPalette::Inactive;
    updatePreviewPalette();
}

void PaletteEditor::handleDisabledRadioClicked()
{
    m_currentColorGroup = QPalette::Disabled;
    updatePreviewPalette();
}

void PaletteEditor::handleComputeRadioClicked()
{
    if (m_compute) {
        return;
    }
    m_ui->paletteView->setColumnHidden(2, true);
    m_ui->paletteView->setColumnHidden(3, true);
    m_compute = true;
    m_paletteModel->setCompute(true);
}

void PaletteEditor::handleDetailsRadioClicked()
{
    if (!m_compute) {
        return;
    }
    const int w = m_ui->paletteView->columnWidth(1);
    m_ui->paletteView->setColumnHidden(2, false);
    m_ui->paletteView->setColumnHidden(3, false);
    auto *const header = m_ui->paletteView->header();
    header->resizeSection(1, w / 3);
    header->resizeSection(2, w / 3);
    header->resizeSection(3, w / 3);
    m_compute = false;
    m_paletteModel->setCompute(false);
}

void PaletteEditor::paletteChanged(const QPalette &palette)
{
    m_modelUpdated = true;
    if (!m_paletteUpdated) {
        setPalette(palette);
    }
    m_modelUpdated = false;
}

void PaletteEditor::buildPalette()
{
    const QColor btn(m_ui->buildButton->color());
    const QPalette temp(btn);
    setPalette(temp);
}

void PaletteEditor::updatePreviewPalette()
{
    const QPalette::ColorGroup g = currentColorGroup();
    // build the preview palette
    const QPalette currentPalette = palette();
    QPalette previewPalette;
    for (int i = QPalette::WindowText; i < QPalette::NColorRoles; ++i) {
        const QPalette::ColorRole r = static_cast<QPalette::ColorRole>(i);
        const QBrush br = currentPalette.brush(g, r);
        previewPalette.setBrush(QPalette::Active, r, br);
        previewPalette.setBrush(QPalette::Inactive, r, br);
        previewPalette.setBrush(QPalette::Disabled, r, br);
    }
}

void PaletteEditor::updateStyledButton()
{
    m_ui->buildButton->setColor(palette().color(QPalette::Active, QPalette::Button));
}

QPalette PaletteEditor::getPalette(QWidget *parent, const QPalette &init, const QPalette &parentPal, int *ok)
{
    PaletteEditor dlg(parent);
    auto parentPalette(parentPal);
    const auto mask = init.
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
                      resolveMask()
#else
                      resolve()
#endif
        ;
    for (int i = 0; i < static_cast<int>(QPalette::NColorRoles); ++i) {
        if (mask & (1 << i)) {
            continue;
        }
        parentPalette.setBrush(
            QPalette::Active, static_cast<QPalette::ColorRole>(i), init.brush(QPalette::Active, static_cast<QPalette::ColorRole>(i)));
        parentPalette.setBrush(
            QPalette::Inactive, static_cast<QPalette::ColorRole>(i), init.brush(QPalette::Inactive, static_cast<QPalette::ColorRole>(i)));
        parentPalette.setBrush(
            QPalette::Disabled, static_cast<QPalette::ColorRole>(i), init.brush(QPalette::Disabled, static_cast<QPalette::ColorRole>(i)));
    }
    dlg.setPalette(init, parentPalette);

    const int result = dlg.exec();
    if (ok) {
        *ok = result;
    }
    return result == QDialog::Accepted ? dlg.palette() : init;
}

PaletteModel::PaletteModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_compute(true)
{
    const QMetaObject *meta = metaObject();
    const QMetaProperty property = meta->property(meta->indexOfProperty("colorRole"));
    const QMetaEnum enumerator = property.enumerator();
    for (int r = QPalette::WindowText; r < QPalette::NColorRoles; ++r) {
        m_roleNames[static_cast<QPalette::ColorRole>(r)] = QLatin1String(enumerator.key(r));
    }
}

int PaletteModel::rowCount(const QModelIndex &) const
{
    return m_roleNames.count();
}

int PaletteModel::columnCount(const QModelIndex &) const
{
    return 4;
}

QVariant PaletteModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= QPalette::NColorRoles || index.column() < 0 || index.column() >= 4) {
        return QVariant();
    }

    if (index.column() == 0) {
        if (role == Qt::DisplayRole) {
            return m_roleNames[static_cast<QPalette::ColorRole>(index.row())];
        }
        if (role == Qt::EditRole) {
            const auto mask = m_palette.
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
                              resolveMask()
#else
                              resolve()
#endif
                ;
            if (mask & (1 << index.row()))
                return true;
            return false;
        }
        return QVariant();
    }
    if (role == BrushRole) {
        return m_palette.brush(columnToGroup(index.column()), static_cast<QPalette::ColorRole>(index.row()));
    }
    return QVariant();
}

bool PaletteModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()) {
        return false;
    }

    if (index.column() != 0 && role == BrushRole) {
        const QBrush br = qvariant_cast<QBrush>(value);
        const QPalette::ColorRole r = static_cast<QPalette::ColorRole>(index.row());
        const QPalette::ColorGroup g = columnToGroup(index.column());
        m_palette.setBrush(g, r, br);

        QModelIndex idxBegin = PaletteModel::index(r, 0);
        QModelIndex idxEnd = PaletteModel::index(r, 3);
        if (m_compute) {
            m_palette.setBrush(QPalette::Inactive, r, br);
            switch (r) {
            case QPalette::WindowText:
            case QPalette::Text:
            case QPalette::ButtonText:
            case QPalette::Base:
                break;
            case QPalette::Dark:
                m_palette.setBrush(QPalette::Disabled, QPalette::WindowText, br);
                m_palette.setBrush(QPalette::Disabled, QPalette::Dark, br);
                m_palette.setBrush(QPalette::Disabled, QPalette::Text, br);
                m_palette.setBrush(QPalette::Disabled, QPalette::ButtonText, br);
                idxBegin = PaletteModel::index(0, 0);
                idxEnd = PaletteModel::index(m_roleNames.count() - 1, 3);
                break;
            case QPalette::Window:
                m_palette.setBrush(QPalette::Disabled, QPalette::Base, br);
                m_palette.setBrush(QPalette::Disabled, QPalette::Window, br);
                idxBegin = PaletteModel::index(QPalette::Base, 0);
                break;
            case QPalette::Highlight:
                break;
            default:
                m_palette.setBrush(QPalette::Disabled, r, br);
                break;
            }
        }
        emit paletteChanged(m_palette);
        emit dataChanged(idxBegin, idxEnd);
        return true;
    }
    if (index.column() == 0 && role == Qt::EditRole) {
        auto mask = m_palette.
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
                    resolveMask()
#else
                    resolve()
#endif
            ;
        const bool isMask = qvariant_cast<bool>(value);
        const int r = index.row();
        if (isMask) {
            mask |= (1 << r);
        } else {
            m_palette.setBrush(
                QPalette::Active, static_cast<QPalette::ColorRole>(r), m_parentPalette.brush(QPalette::Active, static_cast<QPalette::ColorRole>(r)));
            m_palette.setBrush(QPalette::Inactive, static_cast<QPalette::ColorRole>(r),
                m_parentPalette.brush(QPalette::Inactive, static_cast<QPalette::ColorRole>(r)));
            m_palette.setBrush(QPalette::Disabled, static_cast<QPalette::ColorRole>(r),
                m_parentPalette.brush(QPalette::Disabled, static_cast<QPalette::ColorRole>(r)));
            mask &= ~(1 << index.row());
        }
        m_palette.
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
            setResolveMask(mask);
        m_palette = m_palette.resolve(m_palette)
#else
            resolve(mask)
#endif
            ;
        emit paletteChanged(m_palette);
        const QModelIndex idxEnd = PaletteModel::index(r, 3);
        emit dataChanged(index, idxEnd);
        return true;
    }
    return false;
}

Qt::ItemFlags PaletteModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;
    return Qt::ItemIsEditable | Qt::ItemIsEnabled;
}

QVariant PaletteModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        if (section == 0)
            return tr("Color Role");
        if (section == groupToColumn(QPalette::Active))
            return tr("Active");
        if (section == groupToColumn(QPalette::Inactive))
            return tr("Inactive");
        if (section == groupToColumn(QPalette::Disabled))
            return tr("Disabled");
    }
    return QVariant();
}

QPalette PaletteModel::getPalette() const
{
    return m_palette;
}

void PaletteModel::setPalette(const QPalette &palette, const QPalette &parentPalette)
{
    m_parentPalette = parentPalette;
    m_palette = palette;
    const QModelIndex idxBegin = index(0, 0);
    const QModelIndex idxEnd = index(m_roleNames.count() - 1, 3);
    emit dataChanged(idxBegin, idxEnd);
}

QPalette::ColorGroup PaletteModel::columnToGroup(int index) const
{
    if (index == 1)
        return QPalette::Active;
    if (index == 2)
        return QPalette::Inactive;
    return QPalette::Disabled;
}

int PaletteModel::groupToColumn(QPalette::ColorGroup group) const
{
    if (group == QPalette::Active)
        return 1;
    if (group == QPalette::Inactive)
        return 2;
    return 3;
}

BrushEditor::BrushEditor(QWidget *parent)
    : QWidget(parent)
    , m_button(new ColorButton(this))
    , m_changed(false)
{
    auto *const layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_button);
    connect(m_button, &ColorButton::colorChanged, this, &BrushEditor::brushChanged);
    setFocusProxy(m_button);
}

void BrushEditor::setBrush(const QBrush &brush)
{
    m_button->setColor(brush.color());
    m_changed = false;
}

QBrush BrushEditor::brush() const
{
    return QBrush(m_button->color());
}

void BrushEditor::brushChanged()
{
    m_changed = true;
    emit changed(this);
}

bool BrushEditor::changed() const
{
    return m_changed;
}

RoleEditor::RoleEditor(QWidget *parent)
    : QWidget(parent)
    , m_label(new QLabel(this))
    , m_edited(false)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    layout->addWidget(m_label);
    m_label->setAutoFillBackground(true);
    m_label->setIndent(3); // same value as textMargin in QItemDelegate
    setFocusProxy(m_label);

    auto *const button = new QToolButton(this);
    button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    button->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear")));
    button->setIconSize(QSize(8, 8));
    button->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding));
    layout->addWidget(button);
    connect(button, &QAbstractButton::clicked, this, &RoleEditor::emitResetProperty);
}

void RoleEditor::setLabel(const QString &label)
{
    m_label->setText(label);
}

void RoleEditor::setEdited(bool on)
{
    QFont font;
    if (on == true) {
        font.setBold(on);
    }
    m_label->setFont(font);
    m_edited = on;
}

bool RoleEditor::edited() const
{
    return m_edited;
}

void RoleEditor::emitResetProperty()
{
    setEdited(false);
    emit changed(this);
}

ColorDelegate::ColorDelegate(QObject *parent)
    : QItemDelegate(parent)
{
}

QWidget *ColorDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const
{
    if (index.column() == 0) {
        auto *const editor = new RoleEditor(parent);
        connect(editor, &RoleEditor::changed, this, &ColorDelegate::commitData);
        return editor;
    }

    using BrushEditorWidgetSignal = void (BrushEditor::*)(QWidget *);

    auto *const editor = new BrushEditor(parent);
    connect(editor, static_cast<BrushEditorWidgetSignal>(&BrushEditor::changed), this, &ColorDelegate::commitData);
    editor->setFocusPolicy(Qt::NoFocus);
    editor->installEventFilter(const_cast<ColorDelegate *>(this));
    return editor;
}

void ColorDelegate::setEditorData(QWidget *ed, const QModelIndex &index) const
{
    if (index.column() == 0) {
        const auto mask = qvariant_cast<bool>(index.model()->data(index, Qt::EditRole));
        auto *const editor = static_cast<RoleEditor *>(ed);
        editor->setEdited(mask);
        const auto colorName = qvariant_cast<QString>(index.model()->data(index, Qt::DisplayRole));
        editor->setLabel(colorName);
    } else {
        const auto br = qvariant_cast<QBrush>(index.model()->data(index, BrushRole));
        auto *const editor = static_cast<BrushEditor *>(ed);
        editor->setBrush(br);
    }
}

void ColorDelegate::setModelData(QWidget *ed, QAbstractItemModel *model, const QModelIndex &index) const
{
    if (index.column() == 0) {
        const auto *const editor = static_cast<RoleEditor *>(ed);
        const auto mask = editor->edited();
        model->setData(index, mask, Qt::EditRole);
    } else {
        const auto *const editor = static_cast<BrushEditor *>(ed);
        if (editor->changed()) {
            QBrush br = editor->brush();
            model->setData(index, br, BrushRole);
        }
    }
}

void ColorDelegate::updateEditorGeometry(QWidget *ed, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QItemDelegate::updateEditorGeometry(ed, option, index);
    ed->setGeometry(ed->geometry().adjusted(0, 0, -1, -1));
}

void ColorDelegate::paint(QPainter *painter, const QStyleOptionViewItem &opt, const QModelIndex &index) const
{
    QStyleOptionViewItem option = opt;
    const auto mask = qvariant_cast<bool>(index.model()->data(index, Qt::EditRole));
    if (index.column() == 0 && mask) {
        option.font.setBold(true);
    }
    auto br = qvariant_cast<QBrush>(index.model()->data(index, BrushRole));
    if (br.style() == Qt::LinearGradientPattern || br.style() == Qt::RadialGradientPattern || br.style() == Qt::ConicalGradientPattern) {
        painter->save();
        painter->translate(option.rect.x(), option.rect.y());
        painter->scale(option.rect.width(), option.rect.height());
        QGradient gr = *(br.gradient());
        gr.setCoordinateMode(QGradient::LogicalMode);
        br = QBrush(gr);
        painter->fillRect(0, 0, 1, 1, br);
        painter->restore();
    } else {
        painter->save();
        painter->setBrushOrigin(option.rect.x(), option.rect.y());
        painter->fillRect(option.rect, br);
        painter->restore();
    }
    QItemDelegate::paint(painter, option, index);

    const QColor color = static_cast<QRgb>(QApplication::style()->styleHint(QStyle::SH_Table_GridLineColor, &option));
    const QPen oldPen = painter->pen();
    painter->setPen(QPen(color));

    painter->drawLine(option.rect.right(), option.rect.y(), option.rect.right(), option.rect.bottom());
    painter->drawLine(option.rect.x(), option.rect.bottom(), option.rect.right(), option.rect.bottom());
    painter->setPen(oldPen);
}

QSize ColorDelegate::sizeHint(const QStyleOptionViewItem &opt, const QModelIndex &index) const
{
    return QItemDelegate::sizeHint(opt, index) + QSize(4, 4);
}

} // namespace QtUtilities
