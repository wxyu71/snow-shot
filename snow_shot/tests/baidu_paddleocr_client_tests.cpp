#include "snow_shot/network/baidupaddleocrclient.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QHostAddress>
#include <QImage>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

#include <memory>

namespace {
void require(bool condition, const char* message) {
    if (!condition)
        qFatal("PaddleOCR contract failure: %s", message);
}

void directPaddleOcrMarkdownContract() {
    QTcpServer server;
    require(server.listen(QHostAddress::LocalHost), "localhost mock server listens");
    const QString address = QStringLiteral("http://127.0.0.1:%1").arg(server.serverPort());
    int requests = 0;
    bool authenticatedPost = false;
    bool authenticatedPoll = false;
    bool signedDownloadWithoutBearer = false;
    QObject::connect(&server, &QTcpServer::newConnection, &server, [&] {
        while (auto* socket = server.nextPendingConnection()) {
            auto data = std::make_shared<QByteArray>();
            QObject::connect(socket, &QTcpSocket::readyRead, socket,
                             [socket, data, &requests, &authenticatedPost,
                              &authenticatedPoll, &signedDownloadWithoutBearer, address] {
                data->append(socket->readAll());
                const auto divider = data->indexOf("\r\n\r\n");
                if (divider < 0)
                    return;
                const auto header = data->left(divider);
                const auto lengthStart = header.indexOf("content-length:");
                qsizetype expected = 0;
                if (lengthStart >= 0) {
                    const auto line = header.mid(lengthStart).split('\n').first().trimmed();
                    expected = line.mid(QByteArray("content-length:").size()).trimmed().toLongLong();
                }
                if (data->size() - divider - 4 < expected)
                    return;
                ++requests;
                const auto lower = header.toLower();
                QByteArray payload;
                if (header.startsWith("POST /api/v2/ocr/jobs ")) {
                    authenticatedPost = lower.contains("authorization: bearer test-token") &&
                                        data->contains("name=\"model\"") &&
                                        data->contains("PaddleOCR-VL-1.6") &&
                                        data->contains("name=\"file\"");
                    payload = R"({"code":0,"data":{"jobId":"job-one"}})";
                } else if (header.startsWith("GET /api/v2/ocr/jobs/job-one ")) {
                    authenticatedPoll = lower.contains("authorization: bearer test-token");
                    payload = (QStringLiteral(
                        R"({"code":0,"data":{"state":"done","resultUrl":{"markdownUrl":"%1/markdown"}}})")
                        .arg(address)).toUtf8();
                } else if (header.startsWith("GET /markdown ")) {
                    signedDownloadWithoutBearer = !lower.contains("authorization:");
                    payload = "# Summary\n\n| A | B |\n| --- | --- |\n| 1 | 2 |\n";
                } else {
                    payload = R"({"code":500,"message":"unexpected request"})";
                }
                const auto response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n"
                                      "Connection: close\r\nContent-Length: " +
                                      QByteArray::number(payload.size()) + "\r\n\r\n" + payload;
                socket->write(response);
                socket->flush();
                socket->disconnectFromHost();
            });
        }
    });

    snow_shot::network::BaiduPaddleOcrClient client;
    QImage image(32, 16, QImage::Format_RGBA8888);
    image.fill(Qt::white);
    QEventLoop loop;
    QTimer deadline;
    deadline.setSingleShot(true);
    QObject::connect(&deadline, &QTimer::timeout, &loop, &QEventLoop::quit);
    bool finished = false;
    QString markdown;
    QString error;
    client.recognize(image, {address, QStringLiteral("test-token"),
                             QStringLiteral("PaddleOCR-VL-1.6")},
                     [&](QString result, QString failure) {
        finished = true;
        markdown = std::move(result);
        error = std::move(failure);
        loop.quit();
    });
    deadline.start(8000);
    loop.exec();
    require(finished && error.isEmpty(), "cloud OCR request completes");
    require(requests == 3, "submit, poll, and Markdown download each occurred once");
    require(authenticatedPost && authenticatedPoll, "token sent to AI Studio API");
    require(signedDownloadWithoutBearer, "no token disclosed to Markdown download host");
    require(markdown.startsWith(QStringLiteral("# Summary")) &&
            markdown.contains(QStringLiteral("| 1 | 2 |")),
            "Markdown preserves headings and tables");
}

void invalidUrlIsRejected() {
    snow_shot::network::BaiduPaddleOcrClient client;
    QImage image(8, 8, QImage::Format_RGBA8888);
    image.fill(Qt::white);
    bool finished = false;
    client.recognize(image, {QStringLiteral("http://example.com"),
                             QStringLiteral("token"), QStringLiteral("PP-OCRv6")},
                     [&](QString content, QString error) {
        finished = true;
        require(content.isEmpty() && error.contains(QStringLiteral("HTTPS")),
                "unsafe remote HTTP is rejected");
    });
    require(finished, "unsafe URL reports an immediate error");
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    directPaddleOcrMarkdownContract();
    invalidUrlIsRejected();
    return 0;
}
