#ifndef QT_UTILITIES_TESTS_MOCK_REPLY_H
#define QT_UTILITIES_TESTS_MOCK_REPLY_H

#include "../global.h"

#include <QNetworkReply>

#include <memory>

QT_FORWARD_DECLARE_CLASS(QFile)

namespace QtUtilities {

struct MockReplyPrivate;

/*!
 * \brief The MockedReply class provides a fake QNetworkReply which will just return data from a specified buffer.
 */
class QT_UTILITIES_EXPORT MockReply : public QNetworkReply {
    Q_OBJECT

public:
    ~MockReply() override;

public Q_SLOTS:
    void abort() override;

public:
    void close() override;
    qint64 bytesAvailable() const override;
    bool isSequential() const override;
    qint64 size() const override;
    qint64 readData(char *data, qint64 maxlen) override;

    static MockReply *forFile(const QNetworkRequest &request, QFile &&file, int delay = 0);

protected:
    QT_UTILITIES_EXPORT MockReply(const QByteArray &buffer, int delay, QObject *parent = nullptr);

private Q_SLOTS:
    void emitFinished();

private:
    std::unique_ptr<MockReplyPrivate> m_d;
};

} // namespace QtUtilities

#endif // QT_UTILITIES_TESTS_HELPER_H
