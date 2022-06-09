#include "../settingsdialog/settingsdialog.h"
#include "../settingsdialog/optioncategory.h"
#include "../settingsdialog/optioncategorymodel.h"
#include "../settingsdialog/qtsettings.h"

#include <QtTest/QtTest>

using namespace QtUtilities;

class DialogsTests : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void testSettingsDialog();
};

void DialogsTests::testSettingsDialog()
{
    // show single category
    auto settingsDlg = SettingsDialog();
    auto qtSettings = QtSettings();
    settingsDlg.setSingleCategory(qtSettings.category());

    // add another empty category
    auto *const testCategory = new OptionCategory();
    testCategory->setDisplayName(QStringLiteral("Test category"));
    testCategory->setIcon(QIcon::fromTheme(QStringLiteral("preferences")));
    settingsDlg.setSingleCategory(nullptr);
    settingsDlg.categoryModel()->setCategories(QList<OptionCategory *>({testCategory, qtSettings.category()}));
    settingsDlg.showCategory(qtSettings.category());
    settingsDlg.show();
}

QTEST_MAIN(DialogsTests)
#include "dialogs.moc"
