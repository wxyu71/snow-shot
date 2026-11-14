#include "snow_shot/network/baidupaddleocrclient.h"

#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrl>

namespace snow_shot::network {
namespace {
constexpr int kTimeoutMs = 30000;
constexpr int kMaxPolls = 120;
constexpr qsizetype kMaxResultBytes = 8 * 1024 * 1024;

QString responseError(const QJsonObject& object) {
    const auto data = object.value(QStringLiteral("data")).toObject();
    const QString detail = data.value(QStringLiteral("errorMsg")).toString();
    if (!detail.isEmpty())
        return detail;
    const QString message = object.value(QStringLiteral("message")).toString();
    return message.isEmpty() ? QStringLiteral("The PaddleOCR service returned an error") : message;
}
} // namespace

BaiduPaddleOcrClient::BaiduPaddleOcrClient(QObject* parent) : QObject(parent) {
    m_pollTimer.setSingleShot(true);
    m_pollTimer.setInterval(1000);
    connect(&m_pollTimer, &QTimer::timeout, this, [this] { poll(); });
}

BaiduPaddleOcrClient::~BaiduPaddleOcrClient() {
    cancel();
}

bool BaiduPaddleOcrClient::acceptableUrl(const QUrl& url) {
    if (!url.isValid() || url.host().isEmpty() || !url.userInfo().isEmpty() ||
        !url.fragment().isEmpty())
        return false;
    if (url.scheme() == QStringLiteral("https"))
        return true;
    return url.scheme() == QStringLiteral("http") &&
           (url.host() == QStringLiteral("127.0.0.1") ||
            url.host() == QStringLiteral("localhost") ||
            url.host() == QStringLiteral("::1"));
}

void BaiduPaddleOcrClient::cancel() {
    ++m_generation;
    m_pollTimer.stop();
    m_completion = {};
    m_token.clear();
    m_jobId.clear();
    if (m_reply) {
        auto* reply = m_reply.data();
        m_reply.clear();
        reply->abort();
        reply->deleteLater();
    }
}

void BaiduPaddleOcrClient::finish(QString markdown, QString error) {
    m_pollTimer.stop();
    m_token.clear();
    m_jobId.clear();
    auto completion = std::move(m_completion);
    m_completion = {};
    if (completion)
        completion(std::move(markdown), std::move(error));
}

void BaiduPaddleOcrClient::requestFinished(
    QNetworkReply* reply, std::function<void(int, QByteArray, QString)> handler) {
    m_reply = reply;
    const quint64 generation = m_generation;
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, generation, handler = std::move(handler)]() mutable {
                const int status = reply->attribute(
                    QNetworkRequest::HttpStatusCodeAttribute).toInt();
                const QString error = reply->error() == QNetworkReply::NoError
                                          ? QString{} : reply->errorString();
                const QByteArray body = reply->readAll();
                reply->deleteLater();
                if (generation != m_generation || !m_completion)
                    return;
                if (m_reply == reply)
                    m_reply.clear();
                if (body.size() > kMaxResultBytes) {
                    finish({}, QStringLiteral("PaddleOCR response exceeds 8 MiB"));
                    return;
                }
                handler(status, body, error);
            });
}

void BaiduPaddleOcrClient::recognize(const QImage& image, Options options,
                                     Completion completion) {
    cancel();
    m_completion = std::move(completion);
    m_attempts = 0;
    if (!m_completion)
        return;
    if (image.isNull() || options.accessToken.trimmed().isEmpty()) {
        finish({}, QStringLiteral("A screenshot and PaddleOCR Access Token are required"));
        return;
    }
    if (options.model != QStringLiteral("PP-OCRv6") &&
        options.model != QStringLiteral("PP-StructureV3") &&
        options.model != QStringLiteral("PaddleOCR-VL-1.6")) {
        finish({}, QStringLiteral("Unsupported PaddleOCR model"));
        return;
    }

    QString base = options.apiUrl.trimmed();
    while (base.endsWith(u'/'))
        base.chop(1);
    if (!base.endsWith(QStringLiteral("/api/v2/ocr/jobs")))
        base += QStringLiteral("/api/v2/ocr/jobs");
    m_endpoint = QUrl(base);
    if (!acceptableUrl(m_endpoint) || m_endpoint.hasQuery()) {
        finish({}, QStringLiteral("PaddleOCR API URL must be HTTPS (or localhost for testing)"));
        return;
    }
    QByteArray png;
    QBuffer buffer(&png);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG") ||
        png.size() > 15 * 1024 * 1024) {
        finish({}, QStringLiteral("Unable to prepare PNG image (15 MiB limit)"));
        return;
    }
    m_token = options.accessToken.trimmed();

    auto* multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart modelPart;
    modelPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QVariant(QStringLiteral("form-data; name=\"model\"")));
    modelPart.setBody(options.model.toUtf8());
    multipart->append(modelPart);

    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QVariant(QStringLiteral("form-data; name=\"file\"; filename=\"screenshot.png\"")));
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(QStringLiteral("image/png")));
    imagePart.setBody(png);
    multipart->append(imagePart);

    QNetworkRequest request(m_endpoint);
    request.setRawHeader("Authorization", "Bearer " + m_token.toUtf8());
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);
    request.setTransferTimeout(kTimeoutMs);
    auto* reply = m_network.post(request, multipart);
    multipart->setParent(reply);
    requestFinished(reply, [this](int status, QByteArray body, QString error) {
        if (status < 200 || status >= 300 || !error.isEmpty()) {
            finish({}, QStringLiteral("PaddleOCR submission failed (HTTP %1): %2")
                           .arg(status).arg(error));
            return;
        }
        const QJsonDocument json = QJsonDocument::fromJson(body);
        const QJsonObject object = json.object();
        if (!json.isObject() || object.value(QStringLiteral("code")).toInt(-1) != 0) {
            finish({}, responseError(object));
            return;
        }
        m_jobId = object.value(QStringLiteral("data")).toObject()
                      .value(QStringLiteral("jobId")).toVariant().toString();
        if (m_jobId.isEmpty()) {
            finish({}, QStringLiteral("PaddleOCR response has no jobId"));
            return;
        }
        m_pollTimer.start();
    });
}

void BaiduPaddleOcrClient::poll() {
    if (!m_completion)
        return;
    if (++m_attempts > kMaxPolls) {
        finish({}, QStringLiteral("PaddleOCR job timed out"));
        return;
    }
    QUrl url(m_endpoint.toString() + u'/' +
             QString::fromLatin1(QUrl::toPercentEncoding(m_jobId)));
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + m_token.toUtf8());
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);
    request.setTransferTimeout(kTimeoutMs);
    requestFinished(m_network.get(request), [this](int status, QByteArray body, QString error) {
        if (status < 200 || status >= 300 || !error.isEmpty()) {
            finish({}, QStringLiteral("PaddleOCR polling failed (HTTP %1): %2")
                           .arg(status).arg(error));
            return;
        }
        const QJsonDocument json = QJsonDocument::fromJson(body);
        const QJsonObject object = json.object();
        if (!json.isObject() || object.value(QStringLiteral("code")).toInt(-1) != 0) {
            finish({}, responseError(object));
            return;
        }
        const QJsonObject data = object.value(QStringLiteral("data")).toObject();
        const QString state = data.value(QStringLiteral("state")).toString();
        if (state == QStringLiteral("pending") || state == QStringLiteral("running")) {
            m_pollTimer.start();
            return;
        }
        if (state != QStringLiteral("done")) {
            finish({}, responseError(object));
            return;
        }
        const QJsonObject resultUrl = data.value(QStringLiteral("resultUrl")).toObject();
        const QUrl markdownUrl(resultUrl.value(QStringLiteral("markdownUrl")).toString());
        if (!acceptableUrl(markdownUrl)) {
            finish({}, QStringLiteral("PaddleOCR returned no valid Markdown URL"));
            return;
        }
        downloadMarkdown(markdownUrl);
    });
}

void BaiduPaddleOcrClient::downloadMarkdown(const QUrl& url) {
    // Result is a signed download URL; never send the AI Studio Access Token to it.
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);
    request.setTransferTimeout(kTimeoutMs);
    requestFinished(m_network.get(request), [this](int status, QByteArray body, QString error) {
        if (status < 200 || status >= 300 || !error.isEmpty()) {
            finish({}, QStringLiteral("PaddleOCR Markdown download failed (HTTP %1): %2")
                           .arg(status).arg(error));
            return;
        }
        const QString markdown = QString::fromUtf8(body).trimmed();
        if (markdown.isEmpty()) {
            finish({}, QStringLiteral("PaddleOCR returned empty Markdown"));
            return;
        }
        finish(markdown, {});
    });
}

} // namespace snow_shot::network
