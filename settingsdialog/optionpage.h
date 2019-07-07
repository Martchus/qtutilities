#ifndef DIALOGS_OPTIONSPAGE_H
#define DIALOGS_OPTIONSPAGE_H

#include "../global.h"

#include <QObject>
#include <QWidget>

#include <memory>

namespace QtUtilities {

class SettingsDialog;

class QT_UTILITIES_EXPORT OptionPage {
    friend class SettingsDialog;

public:
    explicit OptionPage(QWidget *parentWindow = nullptr);
    virtual ~OptionPage();

    QWidget *parentWindow() const;
    QWidget *widget();
    bool hasBeenShown() const;
    virtual bool apply() = 0;
    virtual void reset() = 0;
    bool matches(const QString &searchKeyWord);
    const QStringList &errors() const;

protected:
    virtual QWidget *setupWidget() = 0;
    QStringList &errors();

private:
    std::unique_ptr<QWidget> m_widget;
    QWidget *m_parentWindow;
    bool m_shown;
    bool m_keywordsInitialized;
    QStringList m_keywords;
    QStringList m_errors;
};

/*!
 * \brief Returns the parent window of the option page.
 */
inline QWidget *OptionPage::parentWindow() const
{
    return m_parentWindow;
}

/*!
 * \brief Returns an indication whether the option page has been shown yet.
 * \remarks If this is true, the method OptionPage::setupWidget() has already
 *          been called.
 */
inline bool OptionPage::hasBeenShown() const
{
    return m_widget != nullptr && m_shown;
}

/*!
 * \brief Returns the errors which haven been occurred when applying the
 * changes.
 */
inline const QStringList &OptionPage::errors() const
{
    return m_errors;
}

/*!
 * \brief Returns the errors which haven been occurred when applying the
 * changes.
 *
 * Error messages should be added when implementing apply() and something goes
 * wrong.
 * In this case, apply() should return false.
 */
inline QStringList &OptionPage::errors()
{
    return m_errors;
}

/*!
 * \class Dialogs::UiFileBasedOptionPage
 * \brief The UiFileBasedOptionPage class is the base class for SettingsDialog
 * pages using UI files
 *        to describe the widget tree.
 *
 * \tparam UiClass Specifies the UI class generated by uic.
 */
template <class UiClass> class QT_UTILITIES_EXPORT UiFileBasedOptionPage : public OptionPage {
public:
    explicit UiFileBasedOptionPage(QWidget *parentWindow = nullptr);
    ~UiFileBasedOptionPage() override;

    bool apply() override = 0;
    void reset() override = 0;

protected:
    QWidget *setupWidget() override;
    UiClass *ui();

private:
    std::unique_ptr<UiClass> m_ui;
};

/*!
 * \brief Constructs a new UI file based option page.
 */
template <class UiClass>
UiFileBasedOptionPage<UiClass>::UiFileBasedOptionPage(QWidget *parentWindow)
    : OptionPage(parentWindow)
{
}

/*!
 * \brief Destroys the option page.
 */
template <class UiClass> UiFileBasedOptionPage<UiClass>::~UiFileBasedOptionPage()
{
}

/*!
 * \brief Inflates the widget for the option page using the UI class.
 */
template <class UiClass> QWidget *UiFileBasedOptionPage<UiClass>::setupWidget()
{
    QWidget *widget = new QWidget();
    if (!m_ui) {
        m_ui.reset(new UiClass);
    }
    m_ui->setupUi(widget);
    return widget;
}

/*!
 * \brief Provides the derived class access to the UI class.
 */
template <class UiClass> inline UiClass *UiFileBasedOptionPage<UiClass>::ui()
{
    return m_ui.get();
}
} // namespace QtUtilities

/*!
 * \brief Declares the base class for a class inheriting from Dialogs::OptionPage.
 */
#define BEGIN_DECLARE_TYPEDEF_OPTION_PAGE(SomeClass) using SomeClass##Base = ::QtUtilities::OptionPage;

/*!
 * \brief Declares a class inheriting from Dialogs::OptionPage in a convenient
 * way.
 * \remarks Must be closed with END_DECLARE_OPTION_PAGE.
 */
#define BEGIN_DECLARE_OPTION_PAGE(SomeClass)                                                                                                         \
    BEGIN_DECLARE_TYPEDEF_OPTION_PAGE(SomeClass)                                                                                                     \
    class QT_UTILITIES_EXPORT SomeClass : public ::QtUtilities::OptionPage {                                                                         \
    public:                                                                                                                                          \
        explicit SomeClass(QWidget *parentWidget = nullptr);                                                                                         \
        ~SomeClass() override;                                                                                                                       \
        bool apply() override;                                                                                                                       \
        void reset() override;                                                                                                                       \
                                                                                                                                                     \
    private:

/*!
 * \brief Declares a class inheriting from Dialogs::OptionPage in a convenient
 * way.
 * \remarks Must be closed with END_DECLARE_OPTION_PAGE.
 */
#define BEGIN_DECLARE_OPTION_PAGE_CUSTOM_CTOR(SomeClass)                                                                                             \
    BEGIN_DECLARE_TYPEDEF_OPTION_PAGE(SomeClass)                                                                                                     \
    class QT_UTILITIES_EXPORT SomeClass : public ::QtUtilities::OptionPage {                                                                         \
    public:                                                                                                                                          \
        ~SomeClass() override;                                                                                                                       \
        bool apply() override;                                                                                                                       \
        void reset() override;                                                                                                                       \
                                                                                                                                                     \
    private:

/*!
 * \brief Declares the base class for a class inheriting from Dialogs::UiFileBasedOptionPage.
 */
#define BEGIN_DECLARE_TYPEDEF_UI_FILE_BASED_OPTION_PAGE(SomeClass)                                                                                   \
    namespace Ui {                                                                                                                                   \
    class SomeClass;                                                                                                                                 \
    }                                                                                                                                                \
    using SomeClass##Base = ::QtUtilities::UiFileBasedOptionPage<Ui::SomeClass>;

/*!
 * \brief Declares a class inheriting from Dialogs::UiFileBasedOptionPage in a
 * convenient way.
 * \remarks Must be closed with END_DECLARE_OPTION_PAGE.
 */
#define BEGIN_DECLARE_UI_FILE_BASED_OPTION_PAGE_CUSTOM_CTOR(SomeClass)                                                                               \
    BEGIN_DECLARE_TYPEDEF_UI_FILE_BASED_OPTION_PAGE(SomeClass)                                                                                       \
    class QT_UTILITIES_EXPORT SomeClass : public ::QtUtilities::UiFileBasedOptionPage<Ui::SomeClass> {                                               \
    public:                                                                                                                                          \
        ~SomeClass() override;                                                                                                                       \
        bool apply() override;                                                                                                                       \
        void reset() override;                                                                                                                       \
                                                                                                                                                     \
    private:

/*!
 * \brief Declares a class inheriting from Dialogs::UiFileBasedOptionPage in a
 * convenient way.
 * \remarks Must be closed with END_DECLARE_OPTION_PAGE.
 */
#define BEGIN_DECLARE_UI_FILE_BASED_OPTION_PAGE(SomeClass)                                                                                           \
    BEGIN_DECLARE_UI_FILE_BASED_OPTION_PAGE_CUSTOM_CTOR(SomeClass)                                                                                   \
public:                                                                                                                                              \
    explicit SomeClass(QWidget *parentWidget = nullptr);                                                                                             \
                                                                                                                                                     \
private:

/*!
 * \brief Must be used after BEGIN_DECLARE_OPTION_PAGE and
 * BEGIN_DECLARE_UI_FILE_BASED_OPTION_PAGE.
 */
#define END_DECLARE_OPTION_PAGE                                                                                                                      \
    }                                                                                                                                                \
    ;

/*!
 * \brief Instantiates a class declared with
 * BEGIN_DECLARE_UI_FILE_BASED_OPTION_PAGE in a convenient way.
 * \remarks Might be required when the class is used by another application.
 */
#define INSTANTIATE_UI_FILE_BASED_OPTION_PAGE(SomeClass)                                                                                             \
    namespace QtUtilities {                                                                                                                          \
    template class UiFileBasedOptionPage<Ui::SomeClass>;                                                                                             \
    }

/*!
 * \brief Instantiates a class declared with
 * BEGIN_DECLARE_UI_FILE_BASED_OPTION_PAGE inside a given namespace in a
 * convenient way.
 * \remarks Might be required when the class is used by another application.
 */
#define INSTANTIATE_UI_FILE_BASED_OPTION_PAGE_NS(SomeNamespace, SomeClass)                                                                           \
    namespace QtUtilities {                                                                                                                          \
    template class UiFileBasedOptionPage<::SomeNamespace::Ui::SomeClass>;                                                                            \
    }

/*!
 * \brief Declares external instantiation of class declared with
 * BEGIN_DECLARE_UI_FILE_BASED_OPTION_PAGE in a convenient way.
 * \remarks Might be required when the class comes from an external library.
 */
#define DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE(SomeClass)                                                                                          \
    namespace QtUtilities {                                                                                                                          \
    namespace Ui {                                                                                                                                   \
    class SomeClass;                                                                                                                                 \
    }                                                                                                                                                \
    extern template class UiFileBasedOptionPage<Ui::SomeClass>;                                                                                      \
    }

/*!
 * \brief Declares external instantiation of class declared with
 * BEGIN_DECLARE_UI_FILE_BASED_OPTION_PAGE inside a given namespace in a
 * convenient way.
 * \remarks Might be required when the class comes from an external library.
 */
#define DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE_NS(SomeNamespace, SomeClass)                                                                        \
    namespace SomeNamespace {                                                                                                                        \
    namespace Ui {                                                                                                                                   \
    class SomeClass;                                                                                                                                 \
    }                                                                                                                                                \
    }                                                                                                                                                \
    namespace QtUtilities {                                                                                                                          \
    extern template class UiFileBasedOptionPage<::SomeNamespace::Ui::SomeClass>;                                                                     \
    }

/*!
 * \brief Declares the method setupWidget() in a convenient way.
 * \remarks Can be used between BEGIN_DECLARE_OPTION_PAGE and
 * END_DECLARE_OPTION_PAGE.
 */
#define DECLARE_SETUP_WIDGETS                                                                                                                        \
protected:                                                                                                                                           \
    QWidget *setupWidget() override;                                                                                                                 \
                                                                                                                                                     \
private:

/*!
 * \brief Declares a class inheriting from Dialogs::OptionPage in a convenient
 * way.
 * \remarks Doesn't allow to declare additional class members.
 */
#define DECLARE_UI_FILE_BASED_OPTION_PAGE(SomeClass)                                                                                                 \
    BEGIN_DECLARE_UI_FILE_BASED_OPTION_PAGE(SomeClass)                                                                                               \
    END_DECLARE_OPTION_PAGE

/*!
 * \brief Declares a class inheriting from Dialogs::OptionPage in a convenient
 * way.
 * \remarks Doesn't allow to declare additional class members.
 */
#define DECLARE_OPTION_PAGE(SomeClass)                                                                                                               \
    BEGIN_DECLARE_OPTION_PAGE(SomeClass)                                                                                                             \
    DECLARE_SETUP_WIDGETS                                                                                                                            \
    END_DECLARE_OPTION_PAGE

/*!
 * \brief Declares a class inheriting from Dialogs::UiFileBasedOptionPage in a
 * convenient way.
 * \remarks Doesn't allow to declare additional class members.
 */
#define DECLARE_UI_FILE_BASED_OPTION_PAGE_CUSTOM_SETUP(SomeClass)                                                                                    \
    BEGIN_DECLARE_UI_FILE_BASED_OPTION_PAGE(SomeClass)                                                                                               \
    DECLARE_SETUP_WIDGETS                                                                                                                            \
    END_DECLARE_OPTION_PAGE

#endif // DIALOGS_OPTIONSPAGE_H
