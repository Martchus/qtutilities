#include "./mockreply.h"

#include <QFile>
#include <QTimer>

#include <algorithm>

namespace QtUtilities {

struct MockReplyPrivate {
    QByteArray bufferArray;
    std::string_view bufferView;
    const char *pos;
    qint64 bytesLeft;
};

MockReply::MockReply(const QByteArray &buffer, int delay, QObject *parent)
    : QNetworkReply(parent)
    , m_d(std::make_unique<MockReplyPrivate>())
{
    m_d->bufferArray = buffer;
    m_d->bufferView = std::string_view(m_d->bufferArray.data(), static_cast<std::size_t>(m_d->bufferArray.size()));
    m_d->pos = m_d->bufferView.data();
    m_d->bytesLeft = static_cast<qint64>(m_d->bufferView.size());

    setOpenMode(QIODevice::ReadOnly);
    QTimer::singleShot(delay, this, &MockReply::emitFinished);
}

MockReply::~MockReply()
{
}

void MockReply::abort()
{
    close();
}

void MockReply::close()
{
}

qint64 MockReply::bytesAvailable() const
{
    return m_d->bytesLeft;
}

bool MockReply::isSequential() const
{
    return true;
}

qint64 MockReply::size() const
{
    return static_cast<qint64>(m_d->bufferView.size());
}

qint64 MockReply::readData(char *data, qint64 maxlen)
{
    if (!m_d->bytesLeft) {
        return -1;
    }
    const auto bytesToRead = std::min<qint64>(m_d->bytesLeft, maxlen);
    if (!bytesToRead) {
        return 0;
    }
    std::copy(m_d->pos, m_d->pos + bytesToRead, data);
    m_d->pos += bytesToRead;
    m_d->bytesLeft -= bytesToRead;
    return bytesToRead;
}

MockReply *MockReply::forFile(const QNetworkRequest &request, QFile &&file, int delay)
{
    auto buffer = QByteArray();
    if (file.exists()) {
        file.open(QFile::ReadOnly);
        buffer = file.readAll();
        file.close();
    }
    auto *const reply = new MockReply(buffer, delay);
    reply->setRequest(request);
    return reply;
}

void MockReply::emitFinished()
{
    if (m_d->bufferView.empty()) {
        setError(QNetworkReply::InternalServerError, QStringLiteral("No mock reply available for this request."));
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, 404);
        setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, QLatin1String("Not found"));
    } else {
        setError(QNetworkReply::NoError, QString());
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, 200);
        setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, QLatin1String("OK"));
    }
    setFinished(true);
    emit finished();
}

} // namespace QtUtilities
