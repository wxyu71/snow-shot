#include "snow_shot/network/baidupaddleocrclient.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QHttpMultiPart>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrl>

#include <utility>
#include <QStringList>

namespace snow_shot::network {
namespace {
constexpr int kTimeoutMs = 30000;
// Follow AI Studio's 5s polling guidance; wait at most ten minutes.
constexpr int kPollIntervalMs = 5000;
constexpr int kMaxPolls = 120;
constexpr qsizetype kMaxResultBytes = 8 * 1024 * 1024;

QString responseError(const QJsonObject& object) {
    const auto data = object.value(QStringLiteral("data")).toObject();
    const QString detail = data.value(QStringLiteral("errorMsg")).toString();
    if (!detail.isEmpty())
        return detail;
    QString message = object.value(QStringLiteral("msg")).toString();
    if (message.isEmpty())
        message = object.value(QStringLiteral("message")).toString();
    return message;
}

QString parseJsonlToMarkdown(const QByteArray& body, QString* error) {
    QStringList parts;
    int pages = 0;
    for (const QByteArray& line : body.split('\n')) {
        if (line.trimmed().isEmpty())
            continue;
        QJsonParseError parseError;
        const QJsonDocument json = QJsonDocument::fromJson(line.trimmed(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !json.isObject()) {
            *error = QCoreApplication::translate("BaiduPaddleOcrClient", "Invalid PaddleOCR JSONL document");
            return {};
        }
        const QJsonObject root = json.object();
        const QJsonObject result = root.value(QStringLiteral("result")).toObject();
        const QJsonArray docPages = result.value(QStringLiteral("layoutParsingResults")).toArray();
        for (const QJsonValue& value : docPages) {
            const QString markdown = value.toObject()
                .value(QStringLiteral("markdown")).toObject()
                .value(QStringLiteral("text")).toString().trimmed();
            if (!markdown.isEmpty()) {
                parts.append(markdown);
                ++pages;
            }
        }
        const QJsonArray ocrPages = result.value(QStringLiteral("ocrResults")).toArray();
        for (const QJsonValue& value : ocrPages) {
            const QJsonArray texts = value.toObject()
                .value(QStringLiteral("prunedResult")).toObject()
                .value(QStringLiteral("rec_texts")).toArray();
            QStringList lines;
            for (const QJsonValue& text : texts) {
                if (text.isString())
                    lines.append(text.toString());
            }
            if (!lines.isEmpty()) {
                parts.append(lines.join(u'\n'));
                ++pages;
            }
        }
    }
    if (!pages)
        *error = QCoreApplication::translate("BaiduPaddleOcrClient", "PaddleOCR JSONL has no recognized text or document Markdown");
    return parts.join(QStringLiteral("\n\n"));
}

QString httpFailure(int status, const QByteArray& body, const QString& networkError) {
    const QJsonObject result = QJsonDocument::fromJson(body).object();
    const int code = result.value(QStringLiteral("code")).toInt(-1);
    if (status == 401 || code == 401)
        return QCoreApplication::translate("BaiduPaddleOcrClient", "AI Studio Access Token rejected (HTTP 401)");
    if (status == 429 || code == 12002)
        return QCoreApplication::translate("BaiduPaddleOcrClient", "AI Studio rate limit (12002 / HTTP 429), retry later");
    if (status == 403 || code == 12001)
        return QCoreApplication::translate("BaiduPaddleOcrClient", "AI Studio daily OCR quota exhausted (12001 / HTTP 403)");
    if (code == 10007)
        return QCoreApplication::translate("BaiduPaddleOcrClient", "AI Studio does not support the selected model (10007)");
    const QString providerError = responseError(result);
    return QCoreApplication::translate("BaiduPaddleOcrClient", "PaddleOCR API failed (HTTP %1, code %2): %3")
        .arg(status).arg(code)
        .arg(!providerError.isEmpty() ? providerError
             : status > 0 ? QCoreApplication::translate("BaiduPaddleOcrClient", "The PaddleOCR service returned an error")
                          : networkError);
}
} // namespace

BaiduPaddleOcrClient::BaiduPaddleOcrClient(QObject* parent) : QObject(parent) {
    m_pollTimer.setSingleShot(true);
    m_pollTimer.setInterval(kPollIntervalMs);
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
    m_rateLimitRetries = 0;
}

void BaiduPaddleOcrClient::finish(QString markdown, QString error) {
    m_pollTimer.stop();
    if (!m_token.isEmpty())
        error.replace(m_token, QStringLiteral("[redacted]"));
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
                    finish({}, QCoreApplication::translate("BaiduPaddleOcrClient", "PaddleOCR response exceeds 8 MiB"));
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
        finish({}, QCoreApplication::translate("BaiduPaddleOcrClient", "A screenshot and PaddleOCR Access Token are required"));
        return;
    }
    if (options.model != QStringLiteral("PP-OCRv6") &&
        options.model != QStringLiteral("PP-StructureV3") &&
        options.model != QStringLiteral("PaddleOCR-VL-1.6")) {
        finish({}, QCoreApplication::translate("BaiduPaddleOcrClient", "Unsupported PaddleOCR model"));
        return;
    }

    QString base = options.apiUrl.trimmed();
    while (base.endsWith(u'/'))
        base.chop(1);
    if (!base.endsWith(QStringLiteral("/api/v2/ocr/jobs")))
        base += QStringLiteral("/api/v2/ocr/jobs");
    m_endpoint = QUrl(base);
    // Loopback fake-server contract tests must not wait five seconds per job.
    m_pollIntervalMs = m_endpoint.scheme() == u"http" ? 10 : kPollIntervalMs;
    m_pollTimer.setInterval(m_pollIntervalMs);
    if (!acceptableUrl(m_endpoint) || m_endpoint.hasQuery()) {
        finish({}, QCoreApplication::translate("BaiduPaddleOcrClient", "PaddleOCR API URL must be HTTPS (or localhost for testing)"));
        return;
    }
    QByteArray png;
    QBuffer buffer(&png);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG") ||
        png.size() > 15 * 1024 * 1024) {
        finish({}, QCoreApplication::translate("BaiduPaddleOcrClient", "Unable to prepare PNG image (15 MiB limit)"));
        return;
    }
    m_token = options.accessToken.trimmed();

    auto* multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart modelPart;
    modelPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QVariant(QStringLiteral("form-data; name=\"model\"")));
    modelPart.setBody(options.model.toUtf8());
    multipart->append(modelPart);

    // Match the AI Studio v2 local-file examples: optionalPayload is a JSON
    // string in multipart form data, not arbitrary pipeline configuration.
    QJsonObject pipeline{
        {QStringLiteral("useDocOrientationClassify"), false},
        {QStringLiteral("useDocUnwarping"), false},
    };
    pipeline.insert(options.model == QStringLiteral("PP-OCRv6")
                        ? QStringLiteral("useTextlineOrientation")
                        : QStringLiteral("useChartRecognition"),
                    false);
    QHttpPart payloadPart;
    payloadPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                          QVariant(QStringLiteral("form-data; name=\"optionalPayload\"")));
    payloadPart.setBody(QJsonDocument(pipeline).toJson(QJsonDocument::Compact));
    multipart->append(payloadPart);

    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QVariant(QStringLiteral("form-data; name=\"file\"; filename=\"screenshot.png\"")));
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(QStringLiteral("image/png")));
    imagePart.setBody(png);
    multipart->append(imagePart);

    QNetworkRequest request(m_endpoint);
    request.setRawHeader("Authorization", "bearer " + m_token.toUtf8());
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);
    request.setTransferTimeout(kTimeoutMs);
    auto* reply = m_network.post(request, multipart);
    multipart->setParent(reply);
    requestFinished(reply, [this](int status, QByteArray body, QString error) {
        if (status < 200 || status >= 300 || !error.isEmpty()) {
            finish({}, httpFailure(status, body, error));
            return;
        }
        const QJsonDocument json = QJsonDocument::fromJson(body);
        const QJsonObject object = json.object();
        if (!json.isObject() || (object.contains(QStringLiteral("code")) &&
                                object.value(QStringLiteral("code")).toInt(-1) != 0)) {
            finish({}, httpFailure(status, body, {}));
            return;
        }
        m_jobId = object.value(QStringLiteral("data")).toObject()
                      .value(QStringLiteral("jobId")).toVariant().toString();
        if (m_jobId.isEmpty()) {
            finish({}, QCoreApplication::translate("BaiduPaddleOcrClient", "PaddleOCR response has no jobId"));
            return;
        }
        m_pollTimer.start(m_pollIntervalMs);
    });
}

void BaiduPaddleOcrClient::poll() {
    if (!m_completion)
        return;
    if (++m_attempts > kMaxPolls) {
        finish({}, QCoreApplication::translate("BaiduPaddleOcrClient", "PaddleOCR job timed out"));
        return;
    }
    QUrl url(m_endpoint.toString() + u'/' +
             QString::fromLatin1(QUrl::toPercentEncoding(m_jobId)));
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "bearer " + m_token.toUtf8());
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);
    request.setTransferTimeout(kTimeoutMs);
    requestFinished(m_network.get(request), [this](int status, QByteArray body, QString error) {
        if (status < 200 || status >= 300 || !error.isEmpty()) {
            if ((status == 429 || QJsonDocument::fromJson(body).object()
                                   .value(QStringLiteral("code")).toInt() == 12002) &&
                m_rateLimitRetries++ < 3) {
                m_pollTimer.start(m_pollIntervalMs * (1 << m_rateLimitRetries));
                return;
            }
            finish({}, httpFailure(status, body, error));
            return;
        }
        const QJsonDocument json = QJsonDocument::fromJson(body);
        const QJsonObject object = json.object();
        if (!json.isObject() || (object.contains(QStringLiteral("code")) &&
                                object.value(QStringLiteral("code")).toInt(-1) != 0)) {
            if (object.value(QStringLiteral("code")).toInt() == 12002 &&
                m_rateLimitRetries++ < 3) {
                m_pollTimer.start(m_pollIntervalMs * (1 << m_rateLimitRetries));
                return;
            }
            finish({}, httpFailure(status, body, {}));
            return;
        }
        const QJsonObject data = object.value(QStringLiteral("data")).toObject();
        const QString state = data.value(QStringLiteral("state")).toString();
        if (state == QStringLiteral("pending") || state == QStringLiteral("running")) {
            m_pollTimer.start(m_pollIntervalMs);
            return;
        }
        if (state != QStringLiteral("done")) {
            const QString detail = responseError(object);
            finish({}, detail.isEmpty() ? QCoreApplication::translate("BaiduPaddleOcrClient", "The PaddleOCR service returned an error")
                                        : detail);
            return;
        }
        // Per AI Studio documentation, the durable job result is JSONL. For
        // PP-OCRv6 it contains ocrResults[].prunedResult.rec_texts; document
        // parsing models return layoutParsingResults[].markdown.text.
        const QJsonObject resultUrl = data.value(QStringLiteral("resultUrl")).toObject();
        const QUrl jsonUrl(resultUrl.value(QStringLiteral("jsonUrl")).toString());
        if (acceptableUrl(jsonUrl)) {
            downloadResult(jsonUrl, true);
            return;
        }
        // Some document-parsing API deployments only return Markdown links.
        const QUrl markdownUrl(resultUrl.value(QStringLiteral("markdownUrl")).toString());
        if (!acceptableUrl(markdownUrl)) {
            finish({}, QCoreApplication::translate("BaiduPaddleOcrClient", "PaddleOCR returned no valid JSONL or Markdown URL"));
            return;
        }
        downloadResult(markdownUrl, false);
    });
}

void BaiduPaddleOcrClient::downloadResult(const QUrl& url, bool jsonl) {
    // The signed result download never carries Authorization. Allow only
    // HTTPS->HTTPS redirects used by object storage, never downgrade to HTTP.
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(kTimeoutMs);
    requestFinished(m_network.get(request), [this, jsonl](int status, QByteArray body, QString error) {
        if (status < 200 || status >= 300 || !error.isEmpty()) {
            finish({}, QCoreApplication::translate("BaiduPaddleOcrClient", "PaddleOCR result download failed (HTTP %1): %2")
                           .arg(status).arg(error));
            return;
        }
        QString parseError;
        const QString markdown = jsonl ? parseJsonlToMarkdown(body, &parseError)
                                      : QString::fromUtf8(body);
        if (!parseError.isEmpty() || markdown.trimmed().isEmpty()) {
            finish({}, parseError.isEmpty()
                           ? QCoreApplication::translate("BaiduPaddleOcrClient", "PaddleOCR returned empty Markdown") : parseError);
            return;
        }
        finish(markdown.trimmed(), {});
    });
}

} // namespace snow_shot::network
