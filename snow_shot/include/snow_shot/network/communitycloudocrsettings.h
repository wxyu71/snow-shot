#ifndef SNOW_SHOT_NETWORK_COMMUNITYCLOUDOCRSETTINGS_H
#define SNOW_SHOT_NETWORK_COMMUNITYCLOUDOCRSETTINGS_H

#include <QString>

namespace snow_shot::network {
struct CommunityCloudOcrSettings {
    QString apiUrl = QStringLiteral("https://paddleocr.aistudio-app.com");
    QString accessToken;
    QString model = QStringLiteral("PaddleOCR-VL-1.6");
    // Stable community values. Keep the provider choice outside the upstream API client so
    // upstream OCR code can be merged without re-introducing hosted-service coupling.
    QString engine = QStringLiteral("paddleocr");
    QString outputFormat = QStringLiteral("markdown");
};
[[nodiscard]] CommunityCloudOcrSettings readCommunityCloudOcrSettings();
[[nodiscard]] bool writeCommunityCloudOcrSettings(const CommunityCloudOcrSettings& values);
} // namespace snow_shot::network

#endif

