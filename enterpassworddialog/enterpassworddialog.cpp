#include "enterpassworddialog.h"
#include "gui/ui_enterpassworddialog.h"

#include <QEvent>
#include <QGraphicsPixmapItem>
#include <QKeyEvent>
#include <QMessageBox>
#include <QGuiApplication>

#ifdef PLATFORM_SPECIFIC_CAPSLOCK_DETECTION
# if defined(Q_OS_WIN32)
#  include <windows.h>
# elif defined(Q_OS_UNIX)
#  include <X11/XKBlib.h>
#  undef KeyPress
#  undef KeyRelease
#  undef FocusIn
#  undef FocusOut
# endif
#endif

namespace Dialogs {

/*!
 * \class Dialogs::EnterPasswordDialog
 * \brief The EnterPasswordDialog class provides a simple dialog to ask the user for a password.
 */

/*!
 * \brief Constructs a password dialog.
 * \param parent Specifies the parent widget.
 */
EnterPasswordDialog::EnterPasswordDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::EnterPasswordDialog)
{
    // setup ui
    m_ui->setupUi(this);
#ifdef Q_OS_WIN32
    setStyleSheet(QStringLiteral("* { font: 9pt \"Segoe UI\"; } #mainWidget { color: black; background-color: white; border: none; } #bottomWidget { background-color: #F0F0F0; border-top: 1px solid #DFDFDF; } QMessageBox QLabel, QInputDialog QLabel, #instructionLabel {font-size: 12pt; color: #003399; }"));
#else
    setStyleSheet(QStringLiteral("#instructionLabel { font-weight: bold; }"));
#endif
    setDescription();
    setPromptForUserName(false);
    setVerificationRequired(false);
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    installEventFilter(this);
    m_ui->userNameLineEdit->installEventFilter(this);
    m_ui->password1LineEdit->installEventFilter(this);
    m_ui->password2LineEdit->installEventFilter(this);
    // capslock key detection
#ifdef PLATFORM_SPECIFIC_CAPSLOCK_DETECTION
    m_capslockPressed = isCapslockPressed();
#else
    m_capslockPressed = false;
#endif
    m_ui->capslockWarningWidget->setVisible(m_capslockPressed);
    // draw icon to capslock warning graphics view
    QIcon icon = QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning, nullptr, this);
    QGraphicsScene* scene = new QGraphicsScene();
    QGraphicsPixmapItem* item = new QGraphicsPixmapItem(icon.pixmap(16, 16));
    scene->addItem(item);
    m_ui->capslockWarningGraphicsView->setScene(scene);
    // connect signals and slots
    connect(m_ui->showPasswordCheckBox, &QCheckBox::clicked, this, &EnterPasswordDialog::updateShowPassword);
    connect(m_ui->noPwCheckBox,  &QCheckBox::clicked, this, &EnterPasswordDialog::updateShowPassword);
    connect(m_ui->confirmPushButton,  &QPushButton::clicked, this, &EnterPasswordDialog::confirm);
    connect(m_ui->abortPushButton, &QPushButton::clicked, this, &EnterPasswordDialog::abort);
    // grab the keyboard
    grabKeyboard();
}

/*!
 * \brief Destroys the password dialog.
 */
EnterPasswordDialog::~EnterPasswordDialog()
{}

/*!
 * \brief Returns the description. The description is shown under the instruction text.
 * \sa setDescription()
 */
QString EnterPasswordDialog::description() const
{
    return m_ui->descLabel->text();
}

/*!
 * \brief Sets the description.
 * \sa description()
 */
void EnterPasswordDialog::setDescription(const QString &description)
{
    m_ui->descLabel->setText(description);
    m_ui->descLabel->setHidden(description.isEmpty());
    adjustSize();
}

/*!
 * \brief Returns whether the dialogs prompts for a user name as well.
 *
 * The dialog does not prompt for a user name by default.
 *
 * \sa setPromptForUserName()
 */
bool EnterPasswordDialog::promtForUserName() const
{
    return !m_ui->userNameLineEdit->isHidden();
}

/*!
 * \brief Sets whethere the dialog prompts for a user name as well.
 * \sa promptForUserName()
 */
void EnterPasswordDialog::setPromptForUserName(bool prompt)
{
    m_ui->userNameLineEdit->setHidden(!prompt);
    adjustSize();
}

/*!
 * \brief Returns an indication whether a verification (password has to be entered twice) is required.
 *
 * \sa EnterPasswordDialog::setVerificationRequired()
 */
bool EnterPasswordDialog::isVerificationRequired() const
{
    return !m_ui->password2LineEdit->isHidden();
}

/*!
 * \brief Returns an indication whether the user is force to enter a password.
 *
 * If no password is required, the user is allowed to skip the dialog without entering
 * a password.
 *
 * \sa EnterPasswordDialog::setPasswordRequired()
 */
bool EnterPasswordDialog::isPasswordRequired() const
{
    return m_ui->noPwCheckBox->isHidden();
}

/*!
 * \brief Sets whether the user is force to enter a password.
 *
 * If no password is required, the user is allowed to skip the dialog without entering
 * a password.
 *
 * \sa EnterPasswordDialog::isPasswordRequired()
 */
void EnterPasswordDialog::setPasswordRequired(bool value)
{
    m_ui->noPwCheckBox->setHidden(value);
    m_ui->noPwCheckBox->setChecked(false);
    adjustSize();
}

/*!
 * \brief Updates the relevant controls to show entered characters or to mask them them.
 *
 * This private slot is called when m_ui->showPasswordCheckBox is clicked.
 */
void EnterPasswordDialog::updateShowPassword()
{
    m_ui->password1LineEdit->setEchoMode(m_ui->showPasswordCheckBox->isChecked()
                                        ? QLineEdit::Normal
                                        : QLineEdit::Password);
    m_ui->password1LineEdit->setEnabled(!m_ui->noPwCheckBox->isChecked());
    m_ui->password2LineEdit->setEnabled(!(m_ui->showPasswordCheckBox->isChecked() || m_ui->noPwCheckBox->isChecked()));
}

/*!
 * \brief Sets whether a verification (password has to be entered twice) is required.
 *
 * \sa EnterPasswordDialog::isVerificationRequired()
 */
void EnterPasswordDialog::setVerificationRequired(bool value)
{
    if(m_instruction.isEmpty()) {
        m_ui->instructionLabel->setText(value ? tr("Enter the new password") : tr("Enter the password"));
    }
    m_ui->password2LineEdit->setHidden(!value);
    adjustSize();
}

/*!
 * \brief Sets the instruction text.
 *
 * \sa EnterPasswordDialog::instruction()
 */
void EnterPasswordDialog::setInstruction(const QString &value)
{
    m_instruction = value;
    if(m_instruction.isEmpty()) {
        m_ui->instructionLabel->setText(isVerificationRequired() ? tr("Enter the new password") : tr("Enter the password"));
    } else {
        m_ui->instructionLabel->setText(value);
    }
    adjustSize();
}

bool EnterPasswordDialog::event(QEvent *event)
{
    switch(event->type()) {
    case QEvent::KeyPress: {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if(keyEvent->key() == Qt::Key_CapsLock) {
            m_capslockPressed = !m_capslockPressed;
        }
        m_ui->capslockWarningWidget->setVisible(m_capslockPressed);
        break;
    }
    default:
        ;
    }
    return QDialog::event(event);
}

/*!
 * \brief Internal method to notice when the capslock key is pressed by the user.
 *
 * Invocation of this method is done by installing the event filter in the constructor.
 */
bool EnterPasswordDialog::eventFilter(QObject *sender, QEvent *event)
{
    switch(event->type()) {
    case QEvent::KeyPress: {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if(keyEvent->key() == Qt::Key_CapsLock) {
            m_capslockPressed = !m_capslockPressed;
        } else {
            QString text = keyEvent->text();
            if(text.length()) {
                QChar firstChar = text.at(0);
                bool shiftPressed = (keyEvent->modifiers() & Qt::ShiftModifier) != 0;
                if((shiftPressed && firstChar.isLower()) || (!shiftPressed && firstChar.isUpper())) {
                    m_capslockPressed = true;
                } else if(firstChar.isLetter()) {
                    m_capslockPressed = false;
                }
            }
        }
        m_ui->capslockWarningWidget->setVisible(m_capslockPressed);
    }
        break;
    case QEvent::FocusIn:
        if(sender == m_ui->userNameLineEdit || sender == m_ui->password1LineEdit || sender == m_ui->password2LineEdit) {
            releaseKeyboard();
            qobject_cast<QWidget *>(sender)->grabKeyboard();
        }
        break;
    case QEvent::FocusOut:
        if(sender == m_ui->userNameLineEdit || sender == m_ui->password1LineEdit || sender == m_ui->password2LineEdit) {
            qobject_cast<QWidget *>(sender)->releaseKeyboard();
            grabKeyboard();
        }
        break;
    default:
        ;
    }
    return false;
}

/*!
 * \brief Sets the dialog status to QDialog::Accepted if a valid password has been enterd.
 *        Displays an error message otherwise.
 *
 * This private slot is called when m_ui->confirmPushButton is clicked.
 */
void EnterPasswordDialog::confirm()
{
    if(!isPasswordRequired() && m_ui->noPwCheckBox->isChecked()) {
        m_password.clear();
        done(QDialog::Accepted);
    } else {
        QString userName = m_ui->userNameLineEdit->text();
        QString password = m_ui->password1LineEdit->text();
        QString repeatedPassword = m_ui->password2LineEdit->text();
        if(promtForUserName() && userName.isEmpty()) {
            QMessageBox::warning(this, windowTitle(), tr("You didn't enter a user name."));
        } else if(password.isEmpty()) {
            QMessageBox::warning(this, windowTitle(), tr("You didn't enter a password."));
        } else {
            if(isVerificationRequired() && (password != repeatedPassword) && !m_ui->showPasswordCheckBox->isChecked()) {
                if(repeatedPassword.isEmpty()) {
                    QMessageBox::warning(this, windowTitle(), tr("You have to enter the new password twice to ensure you enterd it correct."));
                } else {
                    QMessageBox::warning(this, windowTitle(), tr("You mistyped the password."));
                }
            } else {
                m_userName = userName;
                m_password = password;
                done(QDialog::Accepted);
            }
        }
    }
}

/*!
 * \brief Returns an indication whether the capslock key is pressed using platform specific functions.
 *
 * \remarks - Returns always false for unsupported platforms.
 *          - This method is only avialable if the library is built with
 *            PLATFORM_SPECIFIC_CAPSLOCK_DETECTION defined.
 *          - This static function will be used internally to detect whether the capslock key is pressed
 *            when initializing the dialog if available.
 *          - The function requires the application to be linked against X11 on Linux/Unix/Max OS X.
 */
#ifdef PLATFORM_SPECIFIC_CAPSLOCK_DETECTION
bool EnterPasswordDialog::isCapslockPressed()
{
    // platform dependent method of determining if CAPS LOCK is pressed
# if defined(Q_OS_WIN32)
    return GetKeyState(VK_CAPITAL) == 1;
# elif defined(Q_OS_UNIX)
    Display *d = XOpenDisplay((char*)0);
    bool caps_state = false;
    if (d) {
        unsigned n;
        XkbGetIndicatorState(d, XkbUseCoreKbd, &n);
        caps_state = (n & 0x01) == 1;
    }
    return caps_state;
# else
    return false;
# endif
}
#endif

}
