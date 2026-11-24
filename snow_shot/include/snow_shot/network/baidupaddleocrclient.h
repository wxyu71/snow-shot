#ifndef SNOW_SHOT_NETWORK_BAIDUPADDLEOCRCLIENT_H
#define SNOW_SHOT_NETWORK_BAIDUPADDLEOCRCLIENT_H

#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <QUrl>

#include <functional>

namespace snow_shot::network {

// Direct AI Studio OCR v2 client; no upstream hosted-service relay or local model.
class BaiduPaddleOcrClient final : public QObject {
  public:
    struct Options {
        QString apiUrl = QStringLiteral("https://paddleocr.aistudio-app.com");
        QString accessToken;
        QString model = QStringLiteral("PaddleOCR-VL-1.6");
    };
    using Completion = std::function<void(QString markdown, QString error)>;

    explicit BaiduPaddleOcrClient(QObject* parent = nullptr);
    ~BaiduPaddleOcrClient() override;

    void recognize(const QImage& image, Options options, Completion completion);
    void cancel();

  private:
    void poll();
    void downloadResult(const QUrl& url, bool jsonl);
    void requestFinished(QNetworkReply* reply,
                         std::function<void(int status, QByteArray body, QString error)> handler);
    void finish(QString markdown, QString error);
    [[nodiscard]] static bool acceptableUrl(const QUrl& url);

    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    QTimer m_pollTimer;
    QUrl m_endpoint;
    QString m_token;
    QString m_jobId;
    Completion m_completion;
    quint64 m_generation = 0;
    int m_attempts = 0;
    int m_rateLimitRetries = 0;
    int m_pollIntervalMs = 5000;
};

} // namespace snow_shot::network
#endif // SNOW_SHOT_NETWORK_BAIDUPADDLEOCRCLIENT_H
