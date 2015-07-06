#ifndef DIALOGS_DIALOGUTILS_H
#define DIALOGS_DIALOGUTILS_H

#include <c++utilities/application/global.h>

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace Dialogs {

enum class DocumentStatus {
    NoDocument,
    Saved,
    Unsaved
};

QString LIB_EXPORT generateWindowTitle(DocumentStatus documentStatus, const QString &documentPath);

} // namespace Dialogs

#endif // DIALOGS_DIALOGUTILS_H
