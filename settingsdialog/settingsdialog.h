#ifndef DIALOGS_SETTINGSDIALOG_H
#define DIALOGS_SETTINGSDIALOG_H

#include "../global.h"

#include <QDialog>

#include <memory>

namespace QtUtilities {

class OptionCategoryModel;
class OptionCategoryFilterModel;
class OptionCategory;
class OptionPage;

namespace Ui {
class SettingsDialog;
}

class QT_UTILITIES_EXPORT SettingsDialog : public QDialog {
    Q_OBJECT
    Q_PROPERTY(bool tabBarAlwaysVisible READ isTabBarAlwaysVisible WRITE setTabBarAlwaysVisible)

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog() override;
    bool isTabBarAlwaysVisible() const;
    void setTabBarAlwaysVisible(bool value);
    OptionCategoryModel *categoryModel();
    OptionCategory *category(int categoryIndex) const;
    OptionPage *page(int categoryIndex, int pageIndex) const;
    void showCategory(OptionCategory *category);
    void setSingleCategory(OptionCategory *singleCategory);
    QWidget *cornerWidget(Qt::Corner corner = Qt::TopRightCorner) const;
    void setCornerWidget(QWidget *widget, Qt::Corner corner = Qt::TopRightCorner);
    void addHeadingWidget(QWidget *widget);
    void selectPage(int categoryIndex, int pageIndex);

public Q_SLOTS:
    bool apply();
    void reset();

Q_SIGNALS:
    void applied();
    void resetted();

protected:
    bool event(QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private Q_SLOTS:
    void currentCategoryChanged(const QModelIndex &index);
    void updateTabWidget();

private:
    std::unique_ptr<Ui::SettingsDialog> m_ui;
    OptionCategoryModel *m_categoryModel;
    OptionCategoryFilterModel *m_categoryFilterModel;
    OptionCategory *m_currentCategory;
    bool m_tabBarAlwaysVisible;
};

/*!
 * \brief Returns whether the tab bar is always visible.
 *
 * The tab bar is always visible by default.
 *
 * \sa SettingsDialog::setTabBarAlwaysVisible()
 */
inline bool SettingsDialog::isTabBarAlwaysVisible() const
{
    return m_tabBarAlwaysVisible;
}

/*!
 * \brief Returns the category model used by the settings dialog to manage the
 * categories.
 */
inline OptionCategoryModel *SettingsDialog::categoryModel()
{
    return m_categoryModel;
}
} // namespace QtUtilities

#endif // DIALOGS_SETTINGSDIALOG_H
