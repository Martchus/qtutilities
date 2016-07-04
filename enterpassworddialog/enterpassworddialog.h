#ifndef DIALOGS_ENTERPASSWORDDIALOG_H
#define DIALOGS_ENTERPASSWORDDIALOG_H

#include <c++utilities/application/global.h>

#include <QDialog>

#include <memory>

namespace Dialogs {

namespace Ui {
class EnterPasswordDialog;
}

class LIB_EXPORT EnterPasswordDialog : public QDialog
{
    Q_OBJECT
    Q_PROPERTY(QString userName READ userName)
    Q_PROPERTY(QString password READ password)
    Q_PROPERTY(QString description READ description WRITE setDescription)
    Q_PROPERTY(bool promtForUserName READ promtForUserName WRITE setPromptForUserName)
    Q_PROPERTY(bool isVerificationRequired READ isVerificationRequired WRITE setVerificationRequired)
    Q_PROPERTY(bool isPasswordRequired READ isPasswordRequired WRITE setPasswordRequired)
    Q_PROPERTY(QString instruction READ instruction WRITE setInstruction)
    Q_PROPERTY(bool isCapslockPressed READ isCapslockPressed)

public:
    explicit EnterPasswordDialog(QWidget *parent = nullptr);
    ~EnterPasswordDialog();
    const QString &userName() const;
    const QString &password() const;
    QString description() const;
    void setDescription(const QString &description = QString());
    bool promtForUserName() const;
    void setPromptForUserName(bool prompt);
    bool isVerificationRequired() const;
    void setVerificationRequired(bool value);
    bool isPasswordRequired() const;
    void setPasswordRequired(bool value);
    const QString &instruction() const;
    void setInstruction(const QString &value);
    static bool isCapslockPressed();

protected:
    bool event(QEvent *event);
    bool eventFilter(QObject *sender, QEvent *event);

private Q_SLOTS:
    void updateShowPassword();
    void confirm();
    void abort();

private:
    std::unique_ptr<Ui::EnterPasswordDialog> m_ui;
    QString m_userName;
    QString m_password;
    QString m_instruction;
    bool m_capslockPressed;
};

/*!
 * \brief Returns the entered user name.
 */
inline const QString &EnterPasswordDialog::userName() const
{
    return m_userName;
}

/*!
 * \brief Returns the entered password.
 */
inline const QString &EnterPasswordDialog::password() const
{
    return m_password;
}

/*!
 * \brief Returns the instruction text.
 *
 * The instruction text is displayed at the top of the dialog.
 * If the instruction text is empty the default text "Enter the new password"
 * or "Enter the password" (depending on whether the verification is requried or
 * not) displayed.
 *
 * \sa EnterPasswordDialog::setInstruction()
 */
inline const QString &EnterPasswordDialog::instruction() const
{
    return m_instruction;
}

/*!
 * \brief Clears all results and sets the dialog status to QDialog::Rejected.
 *
 * This private slot is called when m_ui->abortPushButton is clicked.
 */
inline void EnterPasswordDialog::abort()
{
    m_password.clear();
    done(QDialog::Rejected);
}

}

#endif // DIALOGS_ENTERPASSWORDDIALOG_H
