#ifndef WIDGETS_PATHSELECTION_H
#define WIDGETS_PATHSELECTION_H

#include "../global.h"

#include <QFileDialog>

QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QCompleter)

namespace QtUtilities {

class ClearLineEdit;

class QT_UTILITIES_EXPORT PathSelection : public QWidget {
    Q_OBJECT

public:
    explicit PathSelection(QWidget *parent = nullptr);

    ClearLineEdit *lineEdit();
    const ClearLineEdit *lineEdit() const;
    void provideCustomFileMode(QFileDialog::FileMode customFileMode);
    void provideCustomFileDialog(QFileDialog *customFileDialog);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void showFileDialog();

private:
    ClearLineEdit *m_lineEdit;
    QPushButton *m_button;
    QFileDialog *m_customDialog;
    QFileDialog::FileMode m_customMode;
    static QCompleter *m_completer;
};

/*!
 * \brief Returns the line edit with the selected path.
 */
inline ClearLineEdit *PathSelection::lineEdit()
{
    return m_lineEdit;
}

/*!
 * \brief Returns the line edit with the selected path.
 */
inline const ClearLineEdit *PathSelection::lineEdit() const
{
    return m_lineEdit;
}

/*!
 * \brief Can be used to provide a custom file mode.
 *
 * The default file mode is QFileDialog::Directory.
 */
inline void PathSelection::provideCustomFileMode(QFileDialog::FileMode customFileMode)
{
    m_customMode = customFileMode;
}

/*!
 * \brief Can be used to provide a custom file dialog.
 *
 * The default file mode is ignored when a custom file dialog has been
 * specified.
 */
inline void PathSelection::provideCustomFileDialog(QFileDialog *customFileDialog)
{
    m_customDialog = customFileDialog;
}
} // namespace QtUtilities

#endif // WIDGETS_PATHSELECTION_H
