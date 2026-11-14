#include "snow_shot/network/baidupaddleocrclient.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QHostAddress>
#include <QImage>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

#include <memory>
#include <utility>

namespace {
void require(bool condition, const char* message) {
    if (!condition)
        qFatal("PaddleOCR API contract failure: %s", message);
}

enum class ReplyMode { Success, Unauthorized, RateLimited, FailedJob, InvalidJsonl };

struct RunResult {
    QString markdown;
    QString error;
    bool finished = false;
    bool submitAuthenticated = false;
    bool pollAuthenticated = false;
    bool downloadIsAnonymous = false;
    int polls = 0;
};

RunResult runCase(const QString& model, ReplyMode mode) {
    QTcpServer server;
    require(server.listen(QHostAddress::LocalHost), "local fake AI Studio server listens");
    const QString root = QStringLiteral("http://127.0.0.1:%1").arg(server.serverPort());
    RunResult outcome;
    QObject::connect(&server, &QTcpServer::newConnection, &server, [&] {
        while (auto* socket = server.nextPendingConnection()) {
            auto payload = std::make_shared<QByteArray>();
            QObject::connect(socket, &QTcpSocket::readyRead, socket,
                    [&, socket, payload, root] {
                payload->append(socket->readAll());
                const qsizetype divide = payload->indexOf("\r\n\r\n");
                if (divide < 0)
                    return;
                const QByteArray headers = payload->left(divide);
                const QByteArray lower = headers.toLower();
                qsizetype expected = 0;
                const qsizetype lengthPos = lower.indexOf("content-length:");
                if (lengthPos >= 0) {
                    const QByteArray length = lower.mid(lengthPos).split('\n').first()
                                              .mid(QByteArray("content-length:").size()).trimmed();
                    expected = length.toLongLong();
                }
                if (payload->size() - divide - 4 < expected)
                    return;
                const bool bearer = lower.contains("authorization: bearer test-token");
                QByteArray body;
                QByteArray status = "200 OK";
                if (headers.startsWith("POST /api/v2/ocr/jobs ")) {
                    outcome.submitAuthenticated = bearer &&
                        payload->contains("name=\"model\"") &&
                        payload->contains(model.toUtf8()) &&
                        payload->contains("name=\"file\"") &&
                        payload->contains("name=\"optionalPayload\"") &&
                        (model != QStringLiteral("PP-StructureV3") ||
                         (payload->contains("useTableRecognition") &&
                          payload->contains("useFormulaRecognition"))) &&
                        (model != QStringLiteral("PaddleOCR-VL-1.6") ||
                         payload->contains("useLayoutDetection"));
                    if (mode == ReplyMode::Unauthorized) {
                        status = "401 Unauthorized";
                        body = R"({"code":401,"msg":"token invalid"})";
                    } else if (mode == ReplyMode::RateLimited) {
                        status = "429 Too Many Requests";
                        body = R"({"code":12002,"msg":"frequency too high"})";
                    } else {
                        body = R"({"code":0,"data":{"jobId":"job-123"}})";
                    }
                } else if (headers.startsWith("GET /api/v2/ocr/jobs/job-123 ")) {
                    outcome.pollAuthenticated = bearer;
                    ++outcome.polls;
                    if (outcome.polls == 1) {
                        body = R"({"code":0,"data":{"state":"running"}})";
                    } else if (mode == ReplyMode::FailedJob) {
                        body = R"({"code":0,"data":{"state":"failed","errorMsg":"unreadable image"}})";
                    } else {
                        body = QStringLiteral(
                            R"({"code":0,"data":{"state":"done","resultUrl":{"jsonUrl":"%1/result"}}})")
                            .arg(root).toUtf8();
                    }
                } else if (headers.startsWith("GET /result ")) {
                    outcome.downloadIsAnonymous = !lower.contains("authorization:");
                    if (mode == ReplyMode::InvalidJsonl) {
                        body = "{not-valid-json}";
                    } else if (model == QStringLiteral("PP-OCRv6")) {
                        body = R"({"result":{"ocrResults":[{"prunedResult":{"rec_texts":["Hello OCR","Total 42"]}}]}})";
                    } else if (model == QStringLiteral("PP-StructureV3")) {
                        body = R"({"result":{"layoutParsingResults":[{"markdown":{"text":"# Invoice\n\n| Item | Price |\n| --- | --- |\n| Book | 42 |\n\n$$E=mc^2$$"}}]}})";
                    } else {
                        body = R"({"result":{"layoutParsingResults":[{"markdown":{"text":"# Mixed Layout\n\nChinese and English\n\n| X | Y |\n| --- | --- |\n| 3 | 4 |\n\n$\\frac{a}{b}$"}}]}})";
                    }
                } else {
                    status = "404 Not Found";
                    body = R"({"code":11001,"msg":"missing job"})";
                }
                const QByteArray response = "HTTP/1.1 " + status +
                    "\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: " +
                    QByteArray::number(body.size()) + "\r\n\r\n" + body;
                socket->write(response);
                socket->flush();
                socket->disconnectFromHost();
            });
        }
    });

    snow_shot::network::BaiduPaddleOcrClient client;
    QImage image(48, 32, QImage::Format_RGBA8888);
    image.fill(Qt::white);
    QEventLoop loop;
    QTimer deadline;
    deadline.setSingleShot(true);
    QObject::connect(&deadline, &QTimer::timeout, &loop, &QEventLoop::quit);
    client.recognize(image, {root, QStringLiteral("test-token"), model},
                     [&](QString markdown, QString error) {
        outcome.finished = true;
        outcome.markdown = std::move(markdown);
        outcome.error = std::move(error);
        loop.quit();
    });
    if (!outcome.finished) {
        deadline.start(4000);
        loop.exec();
    }
    require(outcome.finished, "OCR request completes, never hangs");
    require(outcome.submitAuthenticated, "POST uses the configured token and model");
    return outcome;
}

void checkSuccessfulModels() {
    auto ocr = runCase(QStringLiteral("PP-OCRv6"), ReplyMode::Success);
    require(ocr.error.isEmpty() && ocr.pollAuthenticated && ocr.downloadIsAnonymous,
            "PP-OCRv6 job authenticates poll but not result download");
    require(ocr.markdown == QStringLiteral("Hello OCR\nTotal 42"),
            "PP-OCRv6 JSONL rec_texts become complete plain Markdown text");

    auto structure = runCase(QStringLiteral("PP-StructureV3"), ReplyMode::Success);
    require(structure.error.isEmpty() && structure.downloadIsAnonymous &&
            structure.markdown.contains(QStringLiteral("| Book | 42 |")) &&
            structure.markdown.contains(QStringLiteral("$$E=mc^2$$")),
            "PP-StructureV3 JSONL preserves Markdown tables and formulas");

    auto mixed = runCase(QStringLiteral("PaddleOCR-VL-1.6"), ReplyMode::Success);
    require(mixed.error.isEmpty() && mixed.downloadIsAnonymous &&
            mixed.markdown.startsWith(QStringLiteral("# Mixed Layout")) &&
            mixed.markdown.contains(QStringLiteral("| 3 | 4 |")) &&
            mixed.markdown.contains(QStringLiteral("\\frac{a}{b}")),
            "PaddleOCR-VL-1.6 JSONL preserves mixed text/table/formula");
}

void checkErrors() {
    const QString model = QStringLiteral("PP-OCRv6");
    auto auth = runCase(model, ReplyMode::Unauthorized);
    require(auth.markdown.isEmpty() && auth.error.contains(QStringLiteral("401")),
            "invalid Access Token is reported as HTTP 401");
    auto throttle = runCase(model, ReplyMode::RateLimited);
    require(throttle.markdown.isEmpty() &&
            throttle.error.contains(QStringLiteral("12002")),
            "rate-limit HTTP 429 and vendor code 12002 are actionable");
    auto failed = runCase(model, ReplyMode::FailedJob);
    require(failed.markdown.isEmpty() &&
            failed.error.contains(QStringLiteral("unreadable image")),
            "failed job propagates data.errorMsg");
    auto invalid = runCase(model, ReplyMode::InvalidJsonl);
    require(invalid.markdown.isEmpty() &&
            invalid.error.contains(QStringLiteral("JSONL")),
            "malformed result never copies partial output");
}

void checkUnsafeUrl() {
    snow_shot::network::BaiduPaddleOcrClient client;
    QImage image(10, 10, QImage::Format_RGBA8888);
    image.fill(Qt::white);
    bool finished = false;
    client.recognize(image, {QStringLiteral("http://example.com"),
                             QStringLiteral("token"), QStringLiteral("PP-OCRv6")},
                     [&](QString data, QString error) {
        finished = true;
        require(data.isEmpty() && error.contains(QStringLiteral("HTTPS")),
                "non-local HTTP is rejected without sending credentials");
    });
    require(finished, "unsafe URL reports a synchronous validation error");
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    checkSuccessfulModels();
    checkErrors();
    checkUnsafeUrl();
    return 0;
}
