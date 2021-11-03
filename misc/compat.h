#ifndef QT_UTILITIES_COMPAT_H
#define QT_UTILITIES_COMPAT_H

#include "../global.h"

#include <c++utilities/misc/traits.h>

#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#define QT_UTILITIES_USE_Q_STRING_VIEW
#endif
// note: QStringView has been available since Qt 5.10 but for a consistent ABI/API regardless which
//       Qt 5.x version is used it makes sense to stick to QStringRef when using Qt 5.

#ifdef QT_UTILITIES_USE_Q_STRING_VIEW
#include <QStringView>
#else
#include <QStringRef>
#endif

namespace QtUtilities {

using Utf16CharType =
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    char16_t
#else
    ushort
#endif
    ;

using StringView =
#ifdef QT_UTILITIES_USE_Q_STRING_VIEW
    QStringView
#else
    QStringRef
#endif
    ;

/*!
 * \brief Makes either a QStringView or a QStringRef depending on the Qt version.
 */
inline StringView makeStringView(const QString &str)
{
    return StringView(
#ifndef QT_UTILITIES_USE_Q_STRING_VIEW
        &
#endif
        str);
}

/*!
 * \brief Makes either a QStringView or a QStringRef depending on the Qt version, applying "mid()" parameters.
 */
template <typename PosType1, typename PosType2 = PosType1,
    CppUtilities::Traits::EnableIf<std::is_integral<PosType1>, std::is_signed<PosType1>, std::is_integral<PosType2>, std::is_signed<PosType2>>
        * = nullptr>
inline StringView midRef(const QString &str, PosType1 pos, PosType2 n = -1)
{
#ifdef QT_UTILITIES_USE_Q_STRING_VIEW
    return QStringView(str).mid(pos, n);
#else
    return str.midRef(pos, n);
#endif
}

/*!
 * \brief Splits \a str into QStringViews, QStringRefs or QStrings depending on the Qt version.
 */
template <class... SplitArgs> inline auto splitRef(const QString &str, SplitArgs &&...args)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return QStringView(str).split(std::forward<SplitArgs>(args)...);
#elif QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
    return str.splitRef(std::forward<SplitArgs>(args)...);
#else
    return str.split(std::forward<SplitArgs>(args)...);
#endif
}

} // namespace QtUtilities

#endif // QT_UTILITIES_COMPAT_H
