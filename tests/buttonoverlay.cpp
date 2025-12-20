#include "../widgets/clearcombobox.h"
#include "../widgets/clearlineedit.h"
#include "../widgets/clearplaintextedit.h"
#include "../widgets/clearspinbox.h"
#include "../widgets/iconbutton.h"

#include "../misc/disablewarningsmoc.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QtTest/QtTest>

using namespace QtUtilities;

class ButtonOverlayTests : public QObject {
    Q_OBJECT

private:
    void assertGeneralDefaults(ButtonOverlay &buttonOverlay);
    void changeBasicConfiguration(ButtonOverlay &buttonOverlay);

private Q_SLOTS:
    void testClearLineEdit();
    void testClearComboBox();
    void testClearSpinBox();
    void testClearPlainTextEdit();
};

void ButtonOverlayTests::assertGeneralDefaults(ButtonOverlay &buttonOverlay)
{
    // assert defaults
    QVERIFY2(buttonOverlay.isClearButtonEnabled(), "clear button enabled by default");
    QVERIFY2(!buttonOverlay.isInfoButtonEnabled(), "info button disabled by default");
    QVERIFY2(buttonOverlay.isCleared(), "widget considered cleared by default");
    QVERIFY2(!buttonOverlay.isUsingCustomLayout(), "not using custom layout by default");
}

void ButtonOverlayTests::changeBasicConfiguration(ButtonOverlay &buttonOverlay)
{
    buttonOverlay.setClearButtonEnabled(false);
    QVERIFY2(!buttonOverlay.isClearButtonEnabled(), "clear button disabled");
    buttonOverlay.enableInfoButton(
        QIcon::fromTheme(QStringLiteral("data-information")).pixmap(IconButton::defaultPixmapSize), QStringLiteral("Some info"));
    QVERIFY2(buttonOverlay.isInfoButtonEnabled(), "info button enabled");
}

void ButtonOverlayTests::testClearLineEdit()
{
    auto clearWidget = ClearLineEdit();

    // assert defaults
    assertGeneralDefaults(clearWidget);
    QVERIFY2(static_cast<QLineEdit &>(clearWidget).isClearButtonEnabled(), "clear button enabled via QLineEdit");

    // change configuration
    changeBasicConfiguration(clearWidget);
    clearWidget.setText(QStringLiteral("not cleared anymore"));
    QVERIFY2(!clearWidget.isCleared(), "widget not considered cleared anymore");
    auto customAction = QAction(QIcon::fromTheme(QStringLiteral("edit-copy")).pixmap(IconButton::defaultPixmapSize), QStringLiteral("Copy"));
    clearWidget.addCustomAction(&customAction);
    QVERIFY2(!clearWidget.isUsingCustomLayout(), "not resorted to using custom layout so far");
    QCOMPARE(clearWidget.actions().size(), 2); // more an implementation detail but the QLineEdit should have 2 actions right now

    // test fallback to custom layout
    static_cast<ButtonOverlay &>(clearWidget).setClearButtonEnabled(true);
    auto pushButton = QPushButton(QStringLiteral("Custom widget"));
    clearWidget.addCustomButton(&pushButton);
    QVERIFY2(clearWidget.isUsingCustomLayout(), "resorted to using custom layout due to addCustomButton()");
    QVERIFY2(static_cast<ButtonOverlay &>(clearWidget).isClearButtonEnabled(), "clear button still enabled");
    QVERIFY2(!static_cast<QLineEdit &>(clearWidget).isClearButtonEnabled(), "clear button not enabled via QLineEdit");
    QCOMPARE(clearWidget.actions().size(), 0); // more an implementation detail but the QLineEdit should have been converted to icon buttons
    QCOMPARE(clearWidget.buttonLayout()->count(), 4);

    // change custom action; its fallback icon button should reflect the change
    const auto *const iconButton = qobject_cast<const IconButton *>(clearWidget.buttonLayout()->itemAt(2)->widget());
    QVERIFY2(iconButton, "icon button present");
    QCOMPARE(iconButton->toolTip(), QStringLiteral("Copy"));
    customAction.setText(QStringLiteral("Paste"));
    QCOMPARE(iconButton->toolTip(), QStringLiteral("Paste"));

    // remove buttons again
    static_cast<ButtonOverlay &>(clearWidget).setClearButtonEnabled(false);
    clearWidget.disableInfoButton();
    clearWidget.removeCustomAction(&customAction);
    clearWidget.removeCustomButton(&pushButton);
    QCOMPARE(clearWidget.buttonLayout()->count(), 0);
}

void ButtonOverlayTests::testClearComboBox()
{
    auto clearWidget = ClearComboBox();

    // assert defaults
    assertGeneralDefaults(clearWidget);

    // change configuration
    changeBasicConfiguration(clearWidget);
    clearWidget.setClearButtonEnabled(true);
    clearWidget.setCurrentText(QStringLiteral("not cleared anymore"));
    QVERIFY2(!clearWidget.isCleared(), "widget not considered cleared anymore");
    auto customAction = QAction(QIcon::fromTheme(QStringLiteral("edit-copy")).pixmap(IconButton::defaultPixmapSize), QStringLiteral("Copy"));
    clearWidget.addCustomAction(&customAction);
    QVERIFY2(!clearWidget.isUsingCustomLayout(), "not resorted to using custom layout so far");

    // trigger fallback
    QCOMPARE(clearWidget.buttonLayout()->count(), 3);
}

void ButtonOverlayTests::testClearSpinBox()
{
    auto clearWidget = ClearSpinBox();

    // assert defaults
    assertGeneralDefaults(clearWidget);

    // change configuration
    changeBasicConfiguration(clearWidget);
    clearWidget.setClearButtonEnabled(true);
    clearWidget.setValue(1);
    QVERIFY2(!clearWidget.isCleared(), "widget not considered cleared anymore");
    auto customAction = QAction(QIcon::fromTheme(QStringLiteral("edit-copy")).pixmap(IconButton::defaultPixmapSize), QStringLiteral("Copy"));
    clearWidget.addCustomAction(&customAction);
    QVERIFY2(!clearWidget.isUsingCustomLayout(), "not resorted to using custom layout so far");

    // trigger fallback
    QCOMPARE(clearWidget.buttonLayout()->count(), 3);
}

void ButtonOverlayTests::testClearPlainTextEdit()
{
    auto clearWidget = ClearPlainTextEdit();

    // assert defaults
    QVERIFY2(clearWidget.isClearButtonEnabled(), "clear button enabled by default");
    QVERIFY2(!clearWidget.isInfoButtonEnabled(), "info button disabled by default");
    QVERIFY2(clearWidget.isCleared(), "widget considered cleared by default");
    QVERIFY2(clearWidget.isUsingCustomLayout(), "using custom layout by default");
    QCOMPARE(clearWidget.buttonLayout()->count(), 1);

    // change configuration
    clearWidget.document()->setPlainText(QStringLiteral("not cleared anymore"));
    QVERIFY2(!clearWidget.isCleared(), "widget not considered cleared anymore");
}

QT_UTILITIES_DISABLE_WARNINGS_FOR_MOC_INCLUDE
QTEST_MAIN(ButtonOverlayTests)
#include "buttonoverlay.moc"
